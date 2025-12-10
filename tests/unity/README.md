# Unity Test Framework

## 下载 / Download

Unity 是一个轻量级的 C 语言单元测试框架，适合嵌入式系统。

请从以下地址下载 Unity 框架并将文件放入此目录：

**GitHub**: https://github.com/ThrowTheSwitch/Unity

需要的文件 / Required files:
- `unity.c`
- `unity.h`
- `unity_internals.h`

## 安装方法 / Installation

```bash
# 方法 1: 手动下载
# 从 GitHub 下载并复制以下文件到此目录:
# - src/unity.c
# - src/unity.h
# - src/unity_internals.h

# 方法 2: 使用 Git
git clone https://github.com/ThrowTheSwitch/Unity.git temp_unity
cp temp_unity/src/unity.c .
cp temp_unity/src/unity.h .
cp temp_unity/src/unity_internals.h .
rm -rf temp_unity
```

## 版本要求 / Version

推荐版本 / Recommended: v2.5.0 或更高

## 使用说明 / Usage

```c
#include "unity.h"

void setUp(void) {
    // 每个测试前调用 / Called before each test
}

void tearDown(void) {
    // 每个测试后调用 / Called after each test
}

void test_Example(void) {
    TEST_ASSERT_EQUAL(1, 1);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_Example);
    return UNITY_END();
}
```
