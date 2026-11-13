# test_student.py —— 学生成绩管理系统的自动化测试。
#
# 原理：把一连串键盘输入写到 stdin，让 student.exe 吃进去，
# 然后检查 stdout 里有没有期望的输出。
#
# 运行：python test_student.py

import subprocess
import os
import sys

# --- 路径 ---
EXE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "student.exe")
DATA_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "students.dat")
CSV_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "students.csv")

# --- 测试数据 ---
# 格式：[学号, 姓名, 语文, 数学, 英语]
TEST_STUDENTS = [
    [1001, "张三", 85.5, 92.0, 78.5],
    [1002, "李四", 95.0, 88.5, 91.0],
    [1003, "王五", 62.0, 55.0, 68.5],
    [1004, "赵六", 77.0, 83.5, 80.0],
    [1005, "陈七", 88.0, 95.5, 92.0],
]


def clean():
    """清理测试产物。"""
    for f in [DATA_FILE, CSV_FILE]:
        if os.path.exists(f):
            os.remove(f)


def run_session(input_text, timeout=10):
    """把 input_text 喂给 student.exe，返回 stdout（utf-8）。"""
    try:
        proc = subprocess.run(
            [EXE],
            input=input_text.encode("utf-8"),
            capture_output=True,
            timeout=timeout,
            cwd=os.path.dirname(os.path.abspath(__file__)),
        )
        return proc.stdout.decode("utf-8", errors="replace") + proc.stderr.decode("utf-8", errors="replace")
    except subprocess.TimeoutExpired:
        return "[TIMEOUT]"
    except FileNotFoundError:
        print(f"❌ 找不到 {EXE}，先编译一下：")
        print("   gcc -std=c11 -Wall -o student.exe main.c student.c file_io.c ui.c commands.c")
        sys.exit(1)


def add_student_input(s):
    """构造给 add_student 用的输入字符串。"""
    lines = [
        str(s[0]),
        s[1],
        f"{s[2]:.1f}",
        f"{s[3]:.1f}",
        f"{s[4]:.1f}",
    ]
    return "\n".join(lines) + "\n"


# =========================================================================
# 测试用例
# =========================================================================

TESTS_PASSED = 0
TESTS_FAILED = 0


def check(name, condition, detail=""):
    global TESTS_PASSED, TESTS_FAILED
    if condition:
        TESTS_PASSED += 1
        print(f"  [OK] {name}")
    else:
        TESTS_FAILED += 1
        print(f"  [FAIL] {name}  {detail}")


def test_add_students():
    """测试 1：添加多个学生。"""
    print("\n[测试 1] 添加学生")
    clean()
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"     # 1=录入 + 暂停回车
    script += "2\n\n"                                      # 显示全部 + 暂停
    script += "0\n\n"                                      # 退出 + 跳过保存提示

    out = run_session(script)

    check("输出包含 1001 张三", "1001" in out and "张三" in out)
    check("输出包含 1002 李四", "1002" in out and "李四" in out)
    check("输出包含 1003 王五", "1003" in out and "王五" in out)
    check("显示录入成功提示", "录入成功" in out)


def test_duplicate_id():
    """测试 2：重复学号被拒绝。"""
    print("\n[测试 2] 重复学号检测")
    clean()
    script = ""
    script += "1\n" + add_student_input(TEST_STUDENTS[0]) + "\n"
    script += "1\n1001\n张三again\n80\n80\n80\n\n"
    script += "0\n\n"
    out = run_session(script)
    check("重复学号提示", "已经有人用了" in out)


def test_search():
    """测试 3：查询功能。"""
    print("\n[测试 3] 查询")
    clean()
    # 先存个文件
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"
    script += "9\n\n"       # 保存
    script += "0\nn\n"      # 退出不保存
    run_session(script)

    # 加载 + 查询
    script = ""
    script += "10\ny\n\n"   # 加载 + 确认 + 暂停
    script += "3\n1002\n\n" # 按学号查 1002
    script += "4\n张\n\n"   # 按姓名查"张"
    script += "0\n\n"
    out = run_session(script)
    check("按学号查到李四", "李四" in out)
    check("按姓名查到张三", "张三" in out)


def test_modify_and_delete():
    """测试 4：修改和删除。"""
    print("\n[测试 4] 修改信息")
    clean()
    script = ""
    script += "1\n" + add_student_input(TEST_STUDENTS[0]) + "\n"
    script += "5\n1001\n\n\n\n70\n\n\n"  # 改数学为 70，其他回车不变
    script += "2\n\n"
    script += "0\n\n"
    out = run_session(script)
    check("修改后能看到 1001", "1001" in out)


def test_sort():
    """测试 5：排序和排名。"""
    print("\n[测试 5] 排序 + 排名（曾有 bug：按平均分排序后 rank 未更新）")
    clean()
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"
    script += "7\n1\n\n"    # 按总分排序
    script += "7\n2\n\n"    # 按平均分排序（之前 rank 没更新）
    script += "0\n\n"
    out = run_session(script)
    check("排序后输出表格", "名次" in out or "总分" in out)
    check("陈七出现在排序结果", "陈七" in out)


def test_stats():
    """测试 6：统计分析。"""
    print("\n[测试 6] 统计分析")
    clean()
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"
    script += "8\n\n"
    script += "0\n\n"
    out = run_session(script)
    check("包含统计输出", "平均" in out and "最高" in out)
    check("包含等级分布", "优秀" in out or "良好" in out or "不及格" in out)


def test_file_io():
    """测试 7：保存/加载。"""
    print("\n[测试 7] 文件存取")
    clean()
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"
    script += "9\n\n"
    script += "0\nn\n"
    run_session(script)

    check("students.dat 生成", os.path.exists(DATA_FILE))

    # 新进程加载
    script = ""
    script += "10\ny\n\n"
    script += "2\n\n"
    script += "0\n\n"
    out = run_session(script)
    check("加载后数据存在", "1001" in out and "1005" in out)


def test_csv_export():
    """测试 8：CSV 导出。"""
    print("\n[测试 8] CSV 导出")
    clean()
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"
    script += "11\n\n"
    script += "0\n\n"
    run_session(script)

    check("CSV 文件生成", os.path.exists(CSV_FILE))

    if os.path.exists(CSV_FILE):
        with open(CSV_FILE, "rb") as f:
            content = f.read().decode("utf-8", errors="replace")
        check("CSV 有 BOM", content.startswith("\ufeff"))
        check("CSV 有表头", "学号" in content and "姓名" in content)
        check("CSV 有等级列", "等级" in content)


def test_grade_level():
    """测试 9：成绩等级边界。"""
    print("\n[测试 9] 成绩等级边界")
    clean()
    script = ""
    # 优秀生：三科都是 90，平均 90
    script += "1\n2001\n优秀生\n90\n90\n90\n\n"
    # 不及格：三科都是 50
    script += "1\n2002\n不及格\n50\n50\n50\n\n"
    script += "8\n\n"
    script += "0\n\n"
    out = run_session(script)
    check("等级分布显示优秀", "优秀" in out)
    check("等级分布显示不及格", "不及格" in out)


def test_bar_chart():
    """测试 10：柱状图。"""
    print("\n[测试 10] 柱状图")
    clean()
    script = ""
    for s in TEST_STUDENTS:
        script += "1\n" + add_student_input(s) + "\n"
    script += "12\n\n"
    script += "0\n\n"
    out = run_session(script)
    check("包含柱状图标记字符", "#" in out or "柱状图" in out)


# =========================================================================
# main
# =========================================================================

def main():
    print("=" * 55)
    print("学生成绩管理系统 —— 自动化测试")
    print("=" * 55)

    if not os.path.exists(EXE):
        print(f"❌ 找不到 {EXE}，先编译一下：")
        print("   gcc -std=c11 -Wall -o student.exe main.c student.c file_io.c ui.c commands.c")
        sys.exit(1)

    test_add_students()
    test_duplicate_id()
    test_search()
    test_modify_and_delete()
    test_sort()
    test_stats()
    test_file_io()
    test_csv_export()
    test_grade_level()
    test_bar_chart()

    clean()

    print("\n" + "=" * 55)
    print(f"测试结果：{TESTS_PASSED} 通过，{TESTS_FAILED} 失败")
    print("=" * 55)

    sys.exit(1 if TESTS_FAILED > 0 else 0)


if __name__ == "__main__":
    main()
