> 历史复盘：本文记录最初 Qt 单体版，文中的 include/ 路径和本地业务流程已迁移。当前目录与启动方式以 docs/ARCHITECTURE.md、startup.md 为准。

# AI Interview 项目面试复盘

## 1. 项目一句话

这是一个基于 C++17 和 Qt6 的 AI 智能面试系统。用户可以新建候选人会话、上传 PDF 简历，系统解析简历内容后调用大模型生成技术面试题，并在候选人提交回答后完成评分、追问、下一题推进和最终总结。

面试时可以这样讲：

> 我做的是一个 C++ 桌面端的智能面试系统，核心目标是把简历解析、LLM 出题、回答评估和面试流程管理串成一个完整闭环。项目里我主要关注了服务层抽象、会话状态管理、第三方接口调用、配置管理、日志和异常处理。

## 2. 技术栈

- 语言和构建：C++17、CMake、Conan
- UI 框架：Qt6 Widgets
- JSON：nlohmann_json
- HTTP：libcurl
- 日志：spdlog
- PDF 解析：PoDoFo
- 依赖：Boost、OpenSSL、Zlib、GTest

## 3. 核心模块

### 3.1 UI 层

对应文件：

- `src/main_qt.cpp`
- `src/ui/MainWindow.cpp`
- `src/ui/config_dialog.cpp`

职责：

- 启动 Qt 应用并加载配置
- 新建候选人会话
- 选择 PDF 简历
- 展示面试官问题、候选人回答、进度和总结
- 用后台线程执行耗时任务，避免界面卡死

可以讲的点：

- UI 线程只做界面更新
- LLM 调用和 PDF 解析放到 `std::thread` 中执行
- 线程完成后通过 `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` 回到主线程更新 UI

### 3.2 面试流程层

对应文件：

- `include/interview/interview_manager.h`
- `src/interview/interview_manager.cpp`

职责：

- 管理候选人会话
- 根据默认岗位要求或简历生成问题
- 维护当前题目下标
- 记录候选人回答
- 调用 LLM 评分
- 决定是否追问
- 判断面试是否完成
- 生成最终总结

关键流程：

1. `GenerateQuestionsFromResume()` 解析简历并生成题库
2. `GetFirstQuestion()` 获取第一题
3. 用户提交回答后调用 `RecordAnswer()`
4. `EvaluateAnswer()` 得到分数、反馈、是否追问
5. 如果需要追问，调用 `GetFollowUpQuestion()`
6. 否则调用 `GetNextQuestion()`
7. 最后一题后调用 `GenerateSummary()`

可以讲的设计点：

- 使用 Pimpl 隐藏实现细节，降低头文件依赖
- 用 `InterviewRecord` 记录每轮问答，便于生成报告
- 用 `curr_question_index`、`followup_question`、`has_followed_up` 控制流程
- 对 LLM 评分失败做降级处理，避免整个面试流程中断

### 3.3 LLM 服务层

对应文件：

- `include/services/llm_client.h`
- `src/services/llm_client.cpp`

职责：

- 封装大模型 HTTP API 调用
- 构造 OpenAI 风格的 chat completion 请求
- 解析模型返回内容
- 生成简历驱动问题
- 评估候选人回答
- 生成面试总结

可以讲的点：

- 把外部大模型调用封装在 `LLMClient`，业务层不直接感知 HTTP 细节
- 通过 system prompt 约束模型角色和输出格式
- 要求模型返回 JSON，再用 `nlohmann_json` 转成结构化数据
- 出题、评分、总结三个能力复用同一个底层 `SendMessage()` 和 `SendConversation()`

### 3.4 PDF 解析层

对应文件：

- `include/services/pdf_parser.h`
- `src/services/pdf_parser.cpp`

职责：

- 校验 PDF 文件是否可读取
- 使用 PoDoFo 加载 PDF
- 遍历页面并提取文字
- 根据文本坐标判断换行，尽量保留简历结构

可以讲的点：

- 解析前先做 `IsValidPDF()` 校验
- 解析过程捕获 PoDoFo 异常并转换成业务异常
- 按页面遍历，用 Y 坐标变化判断换行，提高简历文本可读性

### 3.5 公共基础设施

对应文件：

- `include/common/config.h`
- `src/common/config.cpp`
- `include/common/logger.h`
- `src/common/logger.cpp`
- `include/common/utils.h`

职责：

- 配置加载和校验
- 日志初始化
- JSON 提取工具函数

可以讲的点：

- `Config` 使用单例管理运行配置
- `Logger` 封装 spdlog，支持控制台和文件双 sink
- 配置加载后做参数校验，比如 API URL、API key、模型名、temperature、max_tokens

## 4. 项目主流程

可以用这段回答项目流程：

> 用户启动程序后，系统加载本地 JSON 配置并初始化 Qt 窗口。新建会话时，用户输入候选人信息、选择是否上传简历和题目数量。点击开始面试后，系统在后台线程里创建 `InterviewSession`，如果有简历就通过 `PDFParser` 提取文本，再调用 `LLMClient` 生成结构化问题列表。面试过程中，用户每提交一次回答，系统记录回答并调用 LLM 评分，根据返回的 `need_followup` 决定追问还是进入下一题。所有题目完成后，系统把问答记录再次交给 LLM 生成面试总结。

## 5. 可以重点讲的亮点

### 5.1 分层比较清晰

回答抓手：

> 我没有把所有逻辑写在 UI 里，而是拆成 UI 层、会话管理层、服务层和公共基础设施层。UI 负责交互，`InterviewSession` 负责业务流程，`LLMClient` 和 `PDFParser` 负责外部能力封装，这样以后替换模型、换 PDF 解析库或者增加报告导出时影响范围会比较小。

### 5.2 使用 Pimpl 降低耦合

回答抓手：

> `InterviewSession` 和 `LLMClient` 都用了 Pimpl。这样头文件只暴露稳定接口，具体成员变量和第三方库依赖放在 cpp 里。好处是减少编译依赖，也能让调用方更关注能力而不是实现细节。

### 5.3 耗时任务异步化

回答抓手：

> PDF 解析和 LLM 调用都可能比较慢，如果直接在 Qt 主线程里执行会导致窗口无响应。所以我把这类任务放到了后台线程，完成后通过 Qt 的 queued connection 回到主线程更新控件。

### 5.4 LLM 输出结构化

回答抓手：

> 我没有只拿模型的自然语言返回，而是在 prompt 里要求返回 JSON，比如问题列表、评分结果和总结结果。业务层解析 JSON 后用字段驱动流程，比如 `score`、`feedback`、`need_followup`、`followup_question`，这样结果更可控，也方便后续存报告和做统计。

### 5.5 有异常降级思路

回答抓手：

> 外部 API 不稳定时不能让整个流程直接崩掉，所以评分失败时会记录错误日志并给默认中等分数，然后继续流程。这个策略不一定最终产品最优，但体现了对外部依赖失败的兜底意识。

## 6. 面试官可能追问

### Q1：为什么选 C++ 和 Qt，而不是 Web？

回答：

> 这个项目当时定位是本地桌面面试工具，所以选 Qt 可以快速做跨平台 GUI，同时 C++ 对 PDF 解析、本地音频、后续实时语音等能力比较友好。后端岗位面试里我会强调，项目虽然是桌面端，但核心依然是服务分层、状态流转、外部 API 封装、配置和日志这些后端通用工程能力。

### Q2：这个项目的架构怎么设计？

回答：

> 大体是四层。UI 层负责交互和展示，业务层 `InterviewSession` 负责面试流程，服务层 `LLMClient`、`PDFParser` 负责大模型和 PDF 能力，公共层负责配置、日志和工具函数。这样可以把业务流程和外部依赖隔离开，后续扩展更容易。

### Q3：如何保证 UI 不被 LLM 调用卡住？

回答：

> 通过 `std::thread` 把生成问题、评分、总结这类耗时操作放到后台执行。Qt 控件只能在主线程安全更新，所以后台任务完成后通过 `QMetaObject::invokeMethod` 和 `Qt::QueuedConnection` 投递回 UI 线程。

### Q4：LLM 返回不是合法 JSON 怎么办？

回答：

> 现在的实现会从返回内容中提取 JSON 片段再解析，如果解析失败会抛异常或返回默认评估。更稳妥的改进是使用模型的 JSON schema 或 response_format 能力，增加字段校验和重试策略，必要时把原始返回落日志用于排查。

### Q5：为什么要有 `need_followup`？

回答：

> 面试不是简单的一问一答，回答中等但不够深入时追问更有评估价值。模型评分结果里返回 `need_followup` 和 `followup_question`，业务层据此决定是否追问。为了避免无限追问，我还用了 `has_followed_up` 控制每题最多追问一次。

### Q6：如何做配置管理？

回答：

> 配置集中放在 JSON 文件里，启动时通过 `Config::LoadFromFile()` 加载，然后进行必要校验，比如模型名、API URL、token 数和超时时间。实际生产环境里 API key 不应该放在配置文件明文提交，应迁移到环境变量、密钥管理系统或本地未纳管配置。

### Q7：如何设计日志？

回答：

> 项目封装了 spdlog，支持控制台彩色输出和文件持久化。日志主要用于记录配置加载、PDF 解析、LLM 请求响应、面试流程推进和异常信息。后续生产化要注意脱敏，不能把简历全文、回答内容和敏感 token 明文打到日志里。

### Q8：PDF 解析怎么做的？

回答：

> 使用 PoDoFo 加载 PDF，遍历每一页提取文本条目。简历文本如果全部拼成一行会影响模型理解，所以我根据文本条目的 Y 坐标变化插入换行，尽量保留原文结构。

### Q9：这个项目有哪些不足？

回答：

> 我会从工程化角度说几个：第一，配置里不应该有明文密钥；第二，CMake 里有本地绝对路径，比如 PoDoFo 和配置文件路径，迁移性不够；第三，LLM JSON 解析比较依赖 prompt，应该改成严格 schema 和重试；第四，目前自动化测试不足，只有构建能过，`ctest` 没有实际测试；第五，curl 资源可以进一步 RAII 化，避免异常路径泄漏。

### Q10：如果让你重构，你会怎么做？

回答：

> 我会先把外部依赖抽接口，比如 `ILLMClient`、`IResumeParser`，方便 mock 和单元测试。然后把状态流转抽成明确的状态机，避免 UI 层直接参与太多流程判断。配置方面改成环境变量加本地配置组合，敏感信息不入库。网络层会补超时、重试、HTTP 错误分类和 RAII 资源管理。最后补单元测试和集成测试。

### Q11：后端岗位怎么讲这个项目？

回答：

> 我会把它讲成一个 AI 面试业务引擎，而不只讲桌面 UI。后端相关点包括：服务抽象、第三方 HTTP API 封装、JSON 协议设计、业务状态机、异步任务、异常降级、配置校验、日志追踪和报告结构化。这些能力迁移到 Web 后端就是 Controller、Service、Client、Repository、Job Worker 这些常见分层。

### Q12：如果改成 Web 后端，怎么设计？

回答：

> 可以拆成三个服务接口：上传简历并解析、创建面试会话、提交回答并返回下一步动作。后端保存 session 状态、题库和问答记录。LLM 调用放到 service/client 层，耗时任务可以用异步队列。数据库可以存候选人、会话、问题、回答、评分和报告。前端只根据后端返回的 action 展示下一题、追问或总结。

## 7. 明天可以主动承认并转化的风险点

- 明文密钥：当前配置文件里有 API key，生产环境应该改为环境变量或密钥管理
- SSL 校验：当前 libcurl 关闭了 `CURLOPT_SSL_VERIFYPEER`，生产环境必须开启证书校验
- 资源释放：`curl_easy_cleanup` 和 `curl_slist_free_all` 应该用 RAII 包装，确保异常路径也释放
- 绝对路径：CMake、日志目录、配置路径存在本机路径，应该改成相对路径或运行参数
- 测试不足：项目能构建通过，但 `ctest` 当前没有发现测试，需要补充单元测试
- JSON 解析脆弱：当前通过首尾标记提取 JSON，可以改成严格 JSON 模式、schema 校验和重试
- 报告接口：头文件声明了 `GenerateReport()` 和 `SaveReport()`，但当前源码没有实现，可以作为后续完善点

## 8. 2 分钟项目介绍模板

> 这个项目是一个基于 C++17 和 Qt6 的 AI 智能面试系统。它的核心流程是：用户创建候选人会话并上传 PDF 简历，系统用 PoDoFo 提取简历文本，再调用大模型生成结构化技术问题。面试过程中，候选人每提交一次回答，系统会调用 LLM 做评分和反馈，并根据返回的 `need_followup` 决定追问还是进入下一题。最后系统根据所有问答记录生成面试总结。
>
> 架构上我把它拆成 UI 层、业务会话层、服务层和公共基础设施层。UI 层只负责展示和交互；`InterviewSession` 管理题目下标、回答记录、追问和总结；`LLMClient` 封装大模型 HTTP 调用；`PDFParser` 封装简历解析；配置和日志作为公共模块。项目里我比较关注的是外部 API 调用的封装、结构化 JSON 协议、异步执行避免界面阻塞，以及异常情况下的流程兜底。
>
> 如果继续优化，我会重点做四件事：第一，把密钥和本地绝对路径从代码和配置里移走；第二，用 RAII 重构 curl 资源管理并开启 SSL 校验；第三，引入接口抽象和 mock，补单元测试；第四，把面试流程抽成更明确的状态机，方便以后迁移成 Web 后端服务。

## 9. 需要重点复习的 C++ 问题

- 智能指针：`unique_ptr`、`shared_ptr`、`weak_ptr` 的区别和场景
- RAII：为什么能防止资源泄漏，如何封装 curl 这类 C API
- Pimpl：优点、缺点、为什么能减少编译依赖
- 异常安全：构造失败、资源释放、强/基本/无异常保证
- 多线程：UI 线程和后台线程如何通信，为什么不能跨线程直接改 UI
- 原子变量：`std::atomic<bool>` 的使用场景和局限
- JSON 解析：如何做字段校验、错误处理和兼容
- HTTP 调用：超时、重试、状态码处理、SSL 校验、鉴权
- 日志：日志级别、日志脱敏、链路定位
- 配置：本地配置、环境变量、密钥管理、参数校验
- CMake/Conan：依赖管理、target_link_libraries、include path、可移植性
- 单元测试：如何 mock LLMClient 和 PDFParser

## 10. 构建状态

我本地执行过：

```bash
cmake --build cmake-build-debug
ctest --test-dir cmake-build-debug --output-on-failure
```

结果：

- 构建成功
- 有一个警告：`InterviewSession::GenerateSummary()` 并非所有控制路径都有返回值
- 当前没有被 CTest 发现的自动化测试

