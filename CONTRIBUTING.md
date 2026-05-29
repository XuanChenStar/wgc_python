# Contributing to wgc-python

感谢你考虑为 wgc-python 贡献代码！

## 开发环境

- Windows 10 1903+ (Build 18362)
- Visual Studio 2022 + C++ 桌面开发工具集
- Python 3.8+

## 构建

```bash
# 构建 DLL
cd wgc_python_dll
msbuild wgc_python_dll.vcxproj /p:Configuration=Release /p:Platform=x64

# 复制到工作目录
copy x64\Release\wgc_python.dll ..\wgc_python\
```

## 代码规范

### C++
- 遵循现有代码风格：`WGCWindowCapture` 类处理核心逻辑，`WGCExport` 处理 FFI
- 所有公开接口必须通过 `g_managerMutex` 保护
- 错误信息通过 `SetError` / `GetLastError` 机制传递（线程安全）
- 避免在 `FrameArrived` 回调中做重量级操作

### Python
- `capture_one()` 作为推荐入口，屏蔽 Pause/Resume 细节
- `get_frame()` / `release_frame()` 保留零拷贝路径给高级用户
- 函数参数和返回值必须有类型标注
- numpy 数组默认 BGRA 格式

## 测试

```bash
python test.py        # 功能测试
python test_mt.py     # 多线程压测
```

确保修改后所有测试通过。

## 提交流程

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feat/amazing-feature`)
3. 提交修改 (`git commit -m 'feat: ...'`)
4. 推送到分支 (`git push origin feat/amazing-feature`)
5. 创建 Pull Request

### Commit 规范

参考 [Conventional Commits](https://www.conventionalcommits.org/)：

- `feat:` 新功能
- `fix:` 修复
- `docs:` 文档
- `refactor:` 重构
- `perf:` 性能优化
- `test:` 测试
- `chore:` 构建/工具

## 许可证

贡献即代表你同意你的代码将按 MIT 许可证发布。
