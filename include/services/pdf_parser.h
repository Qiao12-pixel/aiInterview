//
// Created by Joe on 26-4-2.
//

#ifndef PDF_PARSER_H
#define PDF_PARSER_H

#include <string>
#include <memory>

namespace interview {
    namespace services {
        class PDFParaser {
        public:
            PDFParaser();
            ~PDFParaser();

            /**
             * @brief 解析PDF并提取文本
             * @param pdf_path PDF文件的路径
             * @return 提取的文本内容
             */
            std::string ExtractText(const std::string& pdf_path);

            /**
             * @brief 检查PDF文件是否有效
             * @param pdf_path 文件路径
             * @return 返回是否有效
             */
            bool IsValidPDF(const std::string& pdf_path);

        private:
            class PDFParaserImpl;
            std::unique_ptr<PDFParaserImpl> pimpl_;
        };
    }
}





#endif //PDF_PARSER_H
