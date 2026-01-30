#include "generation.hpp"
#include "preprocessor.hpp"
#include "arghandler.hpp"

#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    // 参数处理器
    ArgHandler argHandler;
    ArgHandleResult result = argHandler.handleArgs(argc, argv);

    // 读取源代码
    std::string src_codes;
    {
        std::stringstream ss;
        std::fstream input(result.srcFile, std::ios::in);
        if (!input.is_open()) {
            std::cerr << "When: main processing\nError:\n  ";
            std::cerr << "Cannot open source file\a\n";
            std::cerr << "Note:\n  Path may be invalid or A2pC has no access permission.";
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
    ParseInfo pInfo = parser.getParseInfo();

    // 生成器
    Generator generator(program, info, pInfo);
    {
        std::fstream output(result.outputFile, std::ios::out);
        if (!output.is_open()) {
            std::cerr << "When: main processing\nError:\n  ";
            std::cerr << "Cannot open output file\a\n";
            std::cerr << "Note:\n  Path may be invalid or A2pC has no access permission.";
            return -1;
        }
        output << generator.generate();
    }

    // 编译指令
    // std::stringstream command;
    // command << "nasm -f win64 " << result.outputFile << " -o out.obj";
    // system(command.str().c_str());
    // system("gcc out.obj -o out.exe");
}