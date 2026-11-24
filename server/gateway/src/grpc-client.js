import grpc from "@grpc/grpc-js";
import loader from "@grpc/proto-loader";

export function createGrpcClient(config) {
  const definition = loader.loadSync(config.protoPath, {
    keepCase: true,
    enums: String,
    defaults: true,
  });
  const Client =
    grpc.loadPackageDefinition(definition).aiinterview.v1.InterviewService;
  return new Client(config.grpcTarget, grpc.credentials.createInsecure());
}
