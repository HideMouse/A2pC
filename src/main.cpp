#include "generation.hpp"
#include "preprocessor.hpp"

#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "错误的参数列表!!!\a\n";
        return -1;
    }

    std::string src_codes;
    {
        std::stringstream ss;
        std::fstream input(argv[1], std::ios::in);
        if (!input.is_open()) {
            std::cerr << "无法打开源代码文件!!!\a\n";
            return -1;
        }

        ss << input.rdbuf();
        src_codes = ss.str();
    }

    // 预处理器
    Preprocessor preprocessor(std::move(src_codes));
    PreprocessingInfo info = preprocessor.preprocess();
    std::string preprocessedSrc = preprocessor.getPreprocessedSrc();

    // 分词器
    Tokenizer tokenizer(std::move(preprocessedSrc));
    std::vector<Token> tokens = tokenizer.tokenize();

    // 解析器
    Parser parser(std::move(tokens));
    std::unique_ptr<Program> program = parser.parse();

    // 生成器
    Generator generator(program, info);
    {
        std::fstream output("out.asm", std::ios::out);
        output << generator.generate();
    }

    // 编译指令
    system("nasm -f bin out.asm -o out.bin");
    // system("gcc out.obj -o out.exe");
}