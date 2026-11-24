#pragma once
#include <string>
namespace backend {
// 只接收上传内容，不读取客户端文件路径；不在磁盘保存简历。
std::string ParseResumePdf(const std::string &bytes);
} // namespace backend
