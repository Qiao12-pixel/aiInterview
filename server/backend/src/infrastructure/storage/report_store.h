#pragma once
#include "ports/report_store.h"
#include <memory>
namespace backend {
std::shared_ptr<ReportStore> OpenReportStore(const std::string &config_path);
}
