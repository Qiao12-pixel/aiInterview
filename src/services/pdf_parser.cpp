//
// Created by Joe on 26-4-2.
//

#include "../../include/services/pdf_parser.h"
#include "../../include/common/logger.h"
#include <iostream>
#include <podofo/podofo.h>
#include <stdexcept>
#include <sstream>
#include <spdlog/spdlog.h>
#include <cmath>
namespace interview {
    namespace services {
        class PDFParaser::PDFParaserImpl {
        public:

            std::string ExtractText(const std::string& pdf_path) {
                try {
                    PoDoFo::PdfMemDocument document;
                    document.Load(pdf_path);

                    std::ostringstream text_stream;
                    const auto& pages = document.GetPages();
                    unsigned page_count = pages.GetCount();
                    LOG_DEBUG("PDF has {} pages", page_count);
                    //遍历所有界面提取文本
                    for (unsigned i = 0; i < page_count; ++i) {
                        const PoDoFo::PdfPage& page = pages.GetPageAt(i);

                        //提取文本条目

                        std::vector<PoDoFo::PdfTextEntry> entries;
                        page.ExtractTextTo(entries);

                        //利用坐标机制处理换行
                        if (entries.empty()) {
                            continue;
                        }
                        double last_y = entries[0].Y;
                        const double LINE_THRESHOLD = 2.5;

                        //将文本条目转为字符串
                        /*for (const auto& entry : entries) {
                            text_stream << entry.Text << " ";
                        }*/
                        //处理换行
                        for (size_t j = 0; j <entries.size(); ++j) {
                            const auto& entry = entries[j];
                            // 判断：Y坐标变化超过阈值 → 换行
                            if (j > 0 && std::fabs(entry.Y - last_y) > LINE_THRESHOLD) {
                                text_stream << "\n";
                            }
                            text_stream << entry.Text;
                            last_y = entry.Y;
                        }


                        //页面分割
                        if (i < page_count - 1) {
                            text_stream << "\n\n";
                        }
                    }
                    std::string extracted_text = text_stream.str();
                    LOG_INFO("Extracted {} characters from PDF", extracted_text.length());
                    return extracted_text;
                } catch (const PoDoFo::PdfError& e) {
                    LOG_ERROR("PoDoFo error: {}", e.what());
                    throw std::runtime_error("Failed to parse PDF: " + std::string(e.what()));
                } catch (const std::exception& e) {
                    LOG_ERROR("PDF parsing error: {}", e.what());
                    throw std::runtime_error("Failed to parse PDF: " + std::string(e.what()));
                }
            }
            bool IsValidPDF(const std::string& pdf_path) {
                try {
                    PoDoFo::PdfMemDocument document;
                    document.Load(pdf_path.c_str());
                    return true;
                } catch (...) {
                    return false;
                }
            }
        };
        PDFParaser::PDFParaser() : pimpl_(std::make_unique<PDFParaserImpl>()){

        }
        PDFParaser::~PDFParaser() = default;

        std::string PDFParaser::ExtractText(const std::string &pdf_path) {
            return pimpl_->ExtractText(pdf_path);
        }
        bool PDFParaser::IsValidPDF(const std::string &pdf_path) {
            return pimpl_->IsValidPDF(pdf_path);
        }


    }
}
