# 通用软件项目模板

> 适用于 `E:\projects\software` 下的软件项目初始化。规则体系以 `E:\projects\software\AutoLesson` 为当前基线，并链接长期知识库 `E:\projects\_knowledge`。

## 构建与运行

```powershell
# 技术栈确认后补充后端和前端命令
```

## 架构概览

- **项目类型**：Software
- **AI Mode**：engineer
- **参考基线**：`E:\projects\software\AutoLesson`
- **文档模式**：通用软件项目结构，按 `plan/`、`design/`、`tech/`、`test/`、`sql/` 组织。
- **详细设计**：见 [`docs/tech/技术架构文档.md`](docs/tech/技术架构文档.md)、[`docs/tech/数据库设计文档.md`](docs/tech/数据库设计文档.md) 和 [`docs/README.md`](docs/README.md)。

## 每次开始前

在回答或动手前先读取：

1. `AGENTS.md`
2. `.ai-context.md`
3. `.ai-worklog.md`
4. `.ai-learnings.md`
5. `E:\projects\_knowledge\00_index\rule-priority.md`
6. `E:\projects\_knowledge\00_index\project-index.md`
7. `E:\projects\_knowledge\10_domains\software\software-rules.md`
8. 需要更详细上下文时，再按 `docs/README.md` 读取对应文档

读取后用 2-3 行确认当前状态，再继续任务。文件不存在或为空时，说明未找到对应历史状态并从当前任务开始。

## 工作流

- 中文交流，回复简洁明确。
- 先研究上下文，再改代码；编辑前先定位相关文件、接口、调用链和文档。
- 优先做局部修改、搜索替换和小补丁，避免无必要重写整文件。
- 长文件分段读取和编辑；大改前先形成编号检查清单。
- 命令失败后先分析原因，再决定下一步，不盲目重复执行。
- 完成后清理临时测试文件、未使用代码和无效导入。
- 运行时不应留下控制台警告；修复功能时同步处理相关警告。
- 不在 `main`/`master` 分支直接工作；开始任务前切到 `feature/描述-MMDD` 分支。
- 未经用户明确说“提交”或确认提交前，禁止执行 `git add`、`git commit`；未明确要求 push 时禁止 `git push`。

## 编码约定

- 使用项目既有技术栈和目录结构；新增抽象前先确认是否符合现有模式。
- 后端业务逻辑优先放在对应模块、Service 或 Repository 层，入口文件只保留应用启动、路由注册和兼容层。
- 前端页面渲染、数据请求、事件控制应按既有结构拆分，避免把大量逻辑堆进单个页面文件。
- 单个文件超过 200 行时主动评估拆分；React/复杂组件类文件上限参考 300 行，复杂页面按 `tabs/`、`modals/`、`constants.ts`、`utils.ts` 等结构拆分，不向主文件堆叠大量 JSX/模板。
- UI 变更要符合目标产品形态，不随意引入与现有设计系统冲突的布局和交互。
- 对结构化数据优先使用解析器、Schema、迁移脚本或类型定义，不靠临时字符串拼接。
- 数据库结构变更必须写入 `migrations/`，脚本包含 `-- migrate up` 与 `-- migrate down`。

## 文档管理

所有项目文档统一放入 `docs/`，并维护 `docs/README.md` 索引。

- `docs/plan/`：项目计划、里程碑、模块级任务清单。
- `docs/design/`：需求、功能设计、交互流程和产品决策。
- `docs/tech/`：技术架构、API、数据库、部署、运维等技术文档。
- `docs/sql/`：数据库脚本，使用 `01_`、`02_` 等数字前缀保证顺序。
- `docs/test/`：按模块组织的测试记录。

规则：

- 新建、移动或删除 `docs/` 下文档时，必须同步更新 `docs/README.md`。
- 文档必须归入正确子目录，禁止直接散放在 `docs/` 根目录。
- 功能代码修改后，及时更新 `docs/design/`、`docs/tech/` 或对应 API/测试文档。
- `AGENTS.md` 只保留摘要和链接，不重复维护架构、API、数据库的详细正文。
- 通用经验沉淀到 `E:\projects\_knowledge`，项目事实只写本项目 `docs/` 或本地记忆文件。

## 测试与验证

- 每个 API 开发完成后必须进行功能测试，覆盖正常流程和异常情况。
- 测试记录按模块写入 `docs/test/`，格式包含：认证要求、前置条件、请求/响应格式、测试用例表（场景、入参、预期、实际、状态）。
- 常用验证包括类型检查、单元测试、集成测试、接口烟测、页面交互验证和 `git diff --check`。
- 修改核心用户流程后，应优先验证真实页面或真实接口，不只依赖静态检查。

## 安全约束

- 禁止提交 API Key、Secret、`.env` 真实密码、第三方服务密钥、真实用户隐私数据、临时测试文件和构建产物。
- 不修改 `.env.production` 或任何生产环境配置文件。
- `config.json`、`.env` 和本地配置文件可能包含数据库信息或私有路径，提交前必须谨慎检查。
- 密码、token 等敏感信息不得写入日志；前端只暴露非敏感配置。
- 涉及删除文件、清空数据、卸载依赖、覆盖正式资产等破坏性操作时，必须先向用户说明并等待确认。
- 不执行清空数据的 SQL（`TRUNCATE`、`DROP TABLE`），除非用户明确要求并二次确认。

## 会话记忆协议

### 用户说“保存” / “checkpoint” / “暂停” / “存一下”

1. 覆盖写入 `.ai-context.md`，内容包含：当前任务、已完成步骤、下一步计划、阻塞问题；控制在 500 Token 内。
2. 向 `.ai-worklog.md` 追加工作记录，包含：时间、任务目标、主要变更、涉及文件、验证结果、风险与注意、建议提交信息。
3. 保存时禁止修改 `.ai-learnings.md`。
4. 告知用户：“✓ 进度已保存，工作记录已追加”。

### 用户说“结束” / “总结” / “收工”

1. 回顾本次会话，只提取可复用的踩坑规避、架构决策和编码约定。
2. 追加到 `.ai-learnings.md`，不覆盖历史；总量控制在 10 条以内，淘汰过时条目。
3. 压缩 `.ai-context.md` 为最小可恢复摘要。
4. 告知用户：“✓ 经验已记录，进度已压缩”。

### 用户说“保存今天” / “复盘今天” / “会话存档”

1. 先按本项目记忆协议更新 `.ai-context.md` 和 `.ai-worklog.md`。
2. 再按 `E:\projects\_knowledge\40_agent\session-capture-protocol.md` 写入当天跨项目会话摘要。
3. 不复制完整聊天记录，只保存结论、决策、可复用流程、路径、验证结果、下一步和风险。

### 用户说“提交” / “commit” / “git提交”

1. 用 `git rev-parse --show-toplevel` 确认仓库根目录。
2. 读取 `.ai-worklog.md` 最近记录，并检查 `git status`、`git diff --stat`，必要时查看 `git diff`。
3. 检查敏感文件、不应提交文件和临时产物；发现风险先停止并询问用户。
4. 生成 Conventional Commits 信息：`type(scope): subject`，正文说明主要变更、验证结果和风险。
5. 先展示拟提交文件列表和 commit message，等待用户确认后才能执行 `git add` 和 `git commit`。
6. commit 后保留 `.ai-worklog.md`；push 成功后才清空已推送工作记录，仅保留说明和模板。

## 审核触发词

- 用户说“代码审核”时：优先找 bug、风险、回归和缺失测试。
- 用户说“前后端审核”时：逐页核对前端字段、按钮和后端接口，检查接口缺漏、字段缺漏和设计合理性。
- 用户说“数据库审核”时：基于前端页面和后端接口逐项核对数据库表字段，检查字段缺漏和表设计合理性。
- 用户说“文档审核”时：检查文档是否自洽、可执行、位置正确，并与项目目标一致。

## AI Tool Rules

- Codex / generic Agent：`AGENTS.md`
- Windsurf：`.windsurfrules`
- Cursor：`.cursor/rules/*.mdc` 与 `.cursor/commands/*.md`
- Claude：`CLAUDE.md`
- Gemini：`GEMINI.md`

## Overrides

暂无。

## Do Not Apply

- 游戏项目规则不适用于本项目。
- External / Learning / Experiments 领域规则不适用于本项目，除非用户另行指定。
