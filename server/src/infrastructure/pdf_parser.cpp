#include "interview/pdf_parser.h"
#include <poppler-document.h>
#include <poppler-page.h>
#include <memory>
#include <stdexcept>
namespace backend {
std::string ParseResumePdf(const std::string &bytes) {
    if (bytes.empty())
        return {};
    if (bytes.size() > 5 * 1024 * 1024 || bytes.compare(0, 5, "%PDF-") != 0)
        throw std::invalid_argument("简历必须是有效 PDF，大小不超过 5 MiB");
    std::unique_ptr<poppler::document> document(
        poppler::document::load_from_raw_data(bytes.data(), static_cast<int>(bytes.size())));
    if (!document || document->is_locked())
        throw std::invalid_argument("PDF 损坏或已加密");
    if (document->pages() > 20)
        throw std::invalid_argument("简历最多支持 20 页");
    std::string result;
    for (int i = 0; i < document->pages(); ++i) {
        std::unique_ptr<poppler::page> page(document->create_page(i));
        if (!page)
            throw std::invalid_argument("无法读取 PDF 页面");
        const auto text = page->text().to_utf8();
        result.append(text.begin(), text.end());
        result += '\n';
        if (result.size() > 100000)
            throw std::invalid_argument("简历文字超过 100000 字节，请精简内容");
    }
    if (result.find_first_not_of(" \t\r\n") == std::string::npos)
        throw std::invalid_argument("PDF 无可提取文字，扫描件暂不支持 OCR");
    return result;
}
} // namespace backend
