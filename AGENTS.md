# AGENTS.md

<!-- dev-flow:begin -->
## Dev Flow 约定

1. 项目长期事实以 `docs/PROJECT.md` 为准。仅在用户显式调用 `$dev-flow` 时，使用本地 `.dev-flow/TASK.md` 作为当前任务交接记录。
2. 修改前先检查相关代码、当前 Git 分支、`git status` 和既有差异，保护用户原有修改。
3. 一个任务只追求一个可独立验收的结果。范围外事项记录到 `.dev-flow/BACKLOG.md`，不得顺手扩展。
4. 任务未完成时，完成实质里程碑或准备结束回复前，更新任务记录中的进度、验证结果和下一步，再保存包含 Git 指纹的检查点。
5. 完成前必须记录实际验证结果并审查差异；无法运行的检查写明原因和风险。未经明确要求，不提交、不推送、不回滚、不执行破坏性操作。
6. `AGENTS.md` 只保存长期规则和可靠命令，不写入临时任务流水账。
<!-- dev-flow:end -->
## 项目约定

- 当前目标硬件为 ESP8266 NodeMCU v2；主入口为 `WeatherStation-epaper-remote.ino`。
- 使用 Arduino IDE 1.8.9 随附的 ESP8266 3.1.2 工具链，现有构建参数见 `.build/build.options.json`。
- 屏幕驱动涉及硬件时序和低功耗路径；修改后至少完成 Arduino 编译，连接设备时再烧录并观察串口与屏幕。
