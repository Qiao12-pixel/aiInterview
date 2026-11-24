#include "infrastructure/messaging/rabbitmq.h"
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <stdexcept>
namespace backend::messaging {
class Publisher::Impl {
  public:
    amqp_connection_state_t connection = nullptr;
    std::string exchange, routing, queue;
    void Check(const amqp_rpc_reply_t &reply) {
        if (reply.reply_type != AMQP_RESPONSE_NORMAL)
            throw std::runtime_error("RabbitMQ RPC 失败");
    }
    ~Impl() {
        if (connection) {
            amqp_destroy_connection(connection);
        }
    }
    explicit Impl(const nlohmann::json &c) {
        try {
            exchange = c.value("exchange", std::string("aiinterview.events"));
            routing = c.value("routing_key", std::string("report.completed"));
            connection = amqp_new_connection();
            if (!connection)
                throw std::runtime_error("RabbitMQ 初始化失败");
            auto *socket = amqp_tcp_socket_new(connection);
            timeval timeout{3, 0};
            if (!socket ||
                amqp_socket_open_noblock(socket, c.value("host", std::string("127.0.0.1")).c_str(),
                                         c.value("port", 5672), &timeout))
                throw std::runtime_error("RabbitMQ 连接失败");
            amqp_set_rpc_timeout(connection, &timeout);
            Check(amqp_login(connection, c.value("vhost", std::string("/")).c_str(), 0, 131072, 10,
                             AMQP_SASL_METHOD_PLAIN, c.value("user", std::string("guest")).c_str(),
                             c.value("password", std::string("guest")).c_str()));
            amqp_channel_open(connection, 1);
            Check(amqp_get_rpc_reply(connection));
            amqp_exchange_declare(connection, 1, amqp_cstring_bytes(exchange.c_str()),
                                  amqp_cstring_bytes("topic"), 0, 1, 0, 0, amqp_empty_table);
            Check(amqp_get_rpc_reply(connection));
            queue = c.value("queue", std::string("aiinterview.report.completed"));
            amqp_queue_declare(connection, 1, amqp_cstring_bytes(queue.c_str()), 0, 1, 0, 0,
                               amqp_empty_table);
            Check(amqp_get_rpc_reply(connection));
            amqp_queue_bind(connection, 1, amqp_cstring_bytes(queue.c_str()),
                            amqp_cstring_bytes(exchange.c_str()),
                            amqp_cstring_bytes(routing.c_str()), amqp_empty_table);
            Check(amqp_get_rpc_reply(connection));
            amqp_confirm_select(connection, 1);
            Check(amqp_get_rpc_reply(connection));
        } catch (...) {
            if (connection)
                amqp_destroy_connection(connection);
            connection = nullptr;
            throw;
        }
    }
};
Publisher::Publisher(const nlohmann::json &config) : impl_(std::make_unique<Impl>(config)) {}
Publisher::~Publisher() = default;
void Publisher::Publish(const std::string &id, const std::string &payload) {
    amqp_basic_properties_t properties{};
    properties._flags =
        AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG | AMQP_BASIC_MESSAGE_ID_FLAG;
    properties.content_type = amqp_cstring_bytes("application/json");
    properties.delivery_mode = 2;
    properties.message_id = amqp_cstring_bytes(id.c_str());
    const auto status =
        amqp_basic_publish(impl_->connection, 1, amqp_cstring_bytes(impl_->exchange.c_str()),
                           amqp_cstring_bytes(impl_->routing.c_str()), 1, 0, &properties,
                           amqp_bytes_t{payload.size(), const_cast<char *>(payload.data())});
    if (status)
        throw std::runtime_error("RabbitMQ 发布失败");
    amqp_frame_t frame{};
    timeval timeout{5, 0};
    while (true) {
        if (amqp_simple_wait_frame_noblock(impl_->connection, &frame, &timeout) != AMQP_STATUS_OK)
            throw std::runtime_error("RabbitMQ confirm 超时");
        if (frame.frame_type != AMQP_FRAME_METHOD)
            continue;
        if (frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
            return;
        if (frame.payload.method.id == AMQP_BASIC_RETURN_METHOD ||
            frame.payload.method.id == AMQP_BASIC_NACK_METHOD ||
            frame.payload.method.id == AMQP_CHANNEL_CLOSE_METHOD ||
            frame.payload.method.id == AMQP_CONNECTION_CLOSE_METHOD)
            throw std::runtime_error("RabbitMQ 拒绝事件");
    }
}
bool Publisher::ConsumeOnce(
    const std::function<void(const std::string &, const std::string &)> &handler) {
    amqp_basic_get(impl_->connection, 1, amqp_cstring_bytes(impl_->queue.c_str()), 0);
    auto reply = amqp_get_rpc_reply(impl_->connection);
    impl_->Check(reply);
    if (reply.reply.id == AMQP_BASIC_GET_EMPTY_METHOD)
        return false;
    if (reply.reply.id != AMQP_BASIC_GET_OK_METHOD)
        throw std::runtime_error("RabbitMQ 消费响应无效");
    const auto tag = static_cast<amqp_basic_get_ok_t *>(reply.reply.decoded)->delivery_tag;
    amqp_message_t message{};
    reply = amqp_read_message(impl_->connection, 1, &message, 0);
    impl_->Check(reply);
    const std::string payload(static_cast<const char *>(message.body.bytes), message.body.len);
    std::string id;
    if (message.properties._flags & AMQP_BASIC_MESSAGE_ID_FLAG)
        id.assign(static_cast<const char *>(message.properties.message_id.bytes),
                  message.properties.message_id.len);
    amqp_destroy_message(&message);
    try {
        handler(id, payload);
        if (amqp_basic_ack(impl_->connection, 1, tag, 0))
            throw std::runtime_error("RabbitMQ ACK 失败");
    } catch (...) {
        amqp_basic_reject(impl_->connection, 1, tag, 1);
        throw;
    }
    return true;
}

} // namespace backend::messaging
