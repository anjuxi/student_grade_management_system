/*
 * commands.c —— 菜单每个选项的具体实现。
 *
 * 纯"业务+UI"的粘合层：
 * 调 student.c 的逻辑，用 ui.c 显示，自己不干核心计算。
 */

#include "student.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


/* 小工具：数据空的时候打个提示，返回 1 表示真的空、该 return 了。 */
static int no_data(const GradeSystem *sys) {
    if (sys->count == 0) {
        printf("还没有学生记录!\n");
        return 1;
    }
    return 0;
}


/* ---------- 命令 1：录入 ---------- */

void cmd_add(GradeSystem *sys) {
    Student s;
    int i;

    ui_title("录入学生");

    /* 读学号，检查重复。 */
    while (1) {
        s.id = ui_input_int("学号: ");
        if (s.id <= 0) {
            printf("学号得是正整数!\n");
            continue;
        }
        if (sys_find_by_id(sys, s.id) >= 0) {
            printf("学号 %d 已经有人用了!\n", s.id);
            continue;
        }
        break;
    }

    ui_input_name(s.name);
    if (s.name[0] == '\0') return;

    /* 各科成绩。 */
    for (i = 0; i < sys->subject_count; i++) {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "%s: ", sys->subjects[i]);
        s.score[i] = ui_input_score(prompt);
    }

    sys_recalc(&s, sys->subject_count);
    s.rank = 0;

    if (sys_add(sys, &s)) {
        printf("✅ 录入成功! %s，总分 %.1f，平均 %.1f，等级：%s\n",
               s.name, s.sum, s.avg, grade_level(s.avg));
    } else {
        printf("录入失败，可能内存不够!\n");
    }
}


/* ---------- 命令 2：显示全部 ---------- */

void cmd_show_all(const GradeSystem *sys) {
    int i;

    ui_title("学生成绩表");
    if (no_data(sys)) return;

    printf("共 %d 人\n", sys->count);
    ui_table_head(sys->subject_count, sys->subjects);
    for (i = 0; i < sys->count; i++) {
        ui_table_row(&sys->students[i], sys->subject_count);
    }
}


/* ---------- 命令 3：按学号查 ---------- */

void cmd_search_id(const GradeSystem *sys) {
    int id, idx;

    ui_title("按学号查询");
    if (no_data(sys)) return;

    id = ui_input_int("学号: ");
    idx = sys_find_by_id(sys, id);
    if (idx < 0) {
        printf("查不到学号 %d!\n", id);
        return;
    }

    ui_table_head(sys->subject_count, sys->subjects);
    ui_table_row(&sys->students[idx], sys->subject_count);
    printf("  等级：%s\n", grade_level(sys->students[idx].avg));
}


/* ---------- 命令 4：按姓名模糊查 ---------- */

void cmd_search_name(const GradeSystem *sys) {
    char key[V_NAME_LEN];
    int i, found = 0;

    ui_title("按姓名模糊查询");
    if (no_data(sys)) return;

    if (!ui_input_line(key, sizeof(key), "姓名关键字: ") || key[0] == '\0') {
        printf("没输入关键字!\n");
        return;
    }

    ui_table_head(sys->subject_count, sys->subjects);
    for (i = 0; i < sys->count; i++) {
        if (strstr(sys->students[i].name, key) != NULL) {
            ui_table_row(&sys->students[i], sys->subject_count);
            found++;
        }
    }

    if (found == 0) {
        printf("没有名字包含 \"%s\" 的学生!\n", key);
    } else {
        printf("✅ 找到 %d 条记录!\n", found);
    }
}


/* ---------- 命令 5：修改 ---------- */

void cmd_modify(GradeSystem *sys) {
    int id, idx, i;
    char buf[64];

    ui_title("修改信息");
    if (no_data(sys)) return;

    id = ui_input_int("要修改的学号: ");
    idx = sys_find_by_id(sys, id);
    if (idx < 0) {
        printf("没有学号 %d 的学生!\n", id);
        return;
    }

    printf("\n原信息：\n");
    ui_table_head(sys->subject_count, sys->subjects);
    ui_table_row(&sys->students[idx], sys->subject_count);

    printf("\n直接按回车表示不改。\n");

    /* 改姓名。 */
    snprintf(buf, sizeof(buf), "姓名（原 %s）: ", sys->students[idx].name);
    if (ui_input_line(buf, V_NAME_LEN, buf) && buf[0] != '\0') {
        strncpy(sys->students[idx].name, buf, V_NAME_LEN - 1);
        sys->students[idx].name[V_NAME_LEN - 1] = '\0';
    }

    /* 改各科成绩。 */
    for (i = 0; i < sys->subject_count; i++) {
        char prompt[128];
        snprintf(prompt, sizeof(prompt), "%s（原值 %.1f，回车不改）: ",
                 sys->subjects[i], sys->students[idx].score[i]);
        if (!ui_input_line(buf, sizeof(buf), prompt)) break;
        if (buf[0] == '\0') continue;

        /* str_to_score 在 ui.c 是 static，这里直接用 strtod。 */
        {
            char *end;
            double dv = strtod(buf, &end);
            if (end != buf && *end == '\0') {
                if (dv < 0.0) dv = 0.0;
                if (dv > 100.0) dv = 100.0;
                sys->students[idx].score[i] = (float)dv;
            } else {
                printf("输入无效，%s 保持原值!\n", sys->subjects[i]);
            }
        }
    }

    /* 改完重算。 */
    sys_recalc(&sys->students[idx], sys->subject_count);

    printf("\n✅ 修改后：\n");
    ui_table_head(sys->subject_count, sys->subjects);
    ui_table_row(&sys->students[idx], sys->subject_count);
}


/* ---------- 命令 6：删除 ---------- */

void cmd_delete(GradeSystem *sys) {
    int id, idx;
    char ans[8];

    ui_title("删除学生");
    if (no_data(sys)) return;

    id = ui_input_int("要删除的学号: ");
    idx = sys_find_by_id(sys, id);
    if (idx < 0) {
        printf("没有学号 %d 的学生!\n", id);
        return;
    }

    printf("要删除 %s（学号 %d），确定吗？(y/N): ",
           sys->students[idx].name, id);
    if (!ui_input_line(ans, sizeof(ans), NULL)) return;
    if (ans[0] != 'y' && ans[0] != 'Y') {
        printf("已取消删除。\n");
        return;
    }

    sys_remove_at(sys, idx);
    printf("✅ 已删除，还剩 %d 条!\n", sys->count);
}


/* ---------- 命令 7：排序 ---------- */

void cmd_sort(GradeSystem *sys) {
    int i, choice;

    ui_title("排序");
    if (no_data(sys)) return;

    printf("  1. 总分从高到低\n");
    printf("  2. 平均分从高到低\n");
    printf("  3. 学号从小到大\n");
    printf("  4. 按单科成绩降序\n");
    printf("  0. 返回\n");

    choice = ui_input_int("选择: ");

    switch (choice) {
    case 1:
        sys_sort(sys, SORT_SUM, 0);
        cmd_show_all(sys);
        break;
    case 2:
        sys_sort(sys, SORT_AVG, 0);
        cmd_show_all(sys);
        break;
    case 3:
        sys_sort(sys, SORT_ID, 0);
        cmd_show_all(sys);
        break;
    case 4: {
        int sub;
        printf("可选科目：\n");
        for (i = 0; i < sys->subject_count; i++) {
            printf("  %d. %s\n", i + 1, sys->subjects[i]);
        }
        sub = ui_input_int("选哪一科: ");
        if (sub < 1 || sub > sys->subject_count) {
            printf("没有这个科目!\n");
            return;
        }
        sys_sort(sys, SORT_SUBJECT, sub - 1);
        cmd_show_all(sys);
        break;
    }
    default:
        break;
    }
}


/* ---------- 命令 8：统计 ---------- */

void cmd_stats(const GradeSystem *sys) {
    int i, j;
    float total[V_MAX_SUBJECTS];
    float high[V_MAX_SUBJECTS];
    float low[V_MAX_SUBJECTS];
    int   pass[V_MAX_SUBJECTS];
    int   excellent, good, medium, pass_all, fail;
    float sum_all, max_all, min_all;

    ui_title("统计分析");
    if (no_data(sys)) return;

    /* 初始化累加器。 */
    for (j = 0; j < sys->subject_count; j++) {
        total[j] = 0.0f;
        high[j] = -1.0f;
        low[j] = 101.0f;
        pass[j] = 0;
    }
    sum_all = 0.0f;
    max_all = -1.0f;
    min_all = 10000.0f;
    excellent = good = medium = pass_all = fail = 0;

    /* 一轮遍历把所有统计数据收齐。 */
    for (i = 0; i < sys->count; i++) {
        const Student *s = &sys->students[i];
        int ok = 1;

        for (j = 0; j < sys->subject_count; j++) {
            float v = s->score[j];
            total[j] += v;
            if (v > high[j]) high[j] = v;
            if (v < low[j])  low[j]  = v;
            if (v >= V_PASS_LINE) {
                pass[j]++;
            } else {
                ok = 0;
            }
        }
        if (ok) pass_all++;

        sum_all += s->sum;
        if (s->sum > max_all) max_all = s->sum;
        if (s->sum < min_all) min_all = s->sum;

        /* 按平均分分等级。 */
        if (s->avg >= 90) excellent++;
        else if (s->avg >= 80) good++;
        else if (s->avg >= 70) medium++;
        else if (s->avg >= V_PASS_LINE) pass_all++;
        else fail++;
    }

    /* 每科统计。 */
    printf("【各科统计】\n");
    for (j = 0; j < sys->subject_count; j++) {
        printf("  %-8s 平均 %6.1f  最高 %5.1f  最低 %5.1f  及格 %d 人（%.1f%%）\n",
               sys->subjects[j],
               total[j] / sys->count,
               high[j], low[j],
               pass[j], pass[j] * 100.0f / sys->count);
    }

    ui_line();

    /* 总分统计。 */
    printf("【整体统计】\n");
    printf("  总人数 %d  三科全及格 %d 人  全及格率 %.1f%%\n",
           sys->count, pass_all, pass_all * 100.0f / sys->count);
    printf("  总分平均 %.1f  最高 %.1f  最低 %.1f\n",
           sum_all / sys->count, max_all, min_all);

    ui_line();

    /* 等级分布，用字符画柱状图。 */
    printf("【等级分布（按平均分）】\n");
    printf("  优秀（>=90） %d 人  ", excellent);
    for (i = 0; i < excellent; i++) printf("#");
    printf("\n");
    printf("  良好（>=80） %d 人  ", good);
    for (i = 0; i < good; i++) printf("#");
    printf("\n");
    printf("  中等（>=70） %d 人  ", medium);
    for (i = 0; i < medium; i++) printf("#");
    printf("\n");
    printf("  及格（>=60） %d 人  ", pass_all);
    for (i = 0; i < pass_all; i++) printf("#");
    printf("\n");
    printf("  不及格      %d 人  ", fail);
    for (i = 0; i < fail; i++) printf("#");
    printf("\n");
}


/* ---------- 命令 9/10：保存/加载 ---------- */

void cmd_save(const GradeSystem *sys) {
    ui_title("保存数据");
    if (sys_save(sys, V_FILE_NAME)) {
        printf("✅ 已保存 %d 条到 %s!\n", sys->count, V_FILE_NAME);
    } else {
        printf("保存失败，检查一下文件权限!\n");
    }
}


void cmd_load(GradeSystem *sys) {
    char ans[8];

    ui_title("从文件读取");

    /* 覆盖前问一下，防止误操作把内存里的数据冲没。 */
    if (sys->count > 0) {
        printf("当前有 %d 条记录，加载会覆盖。确定吗？(y/N): ", sys->count);
        if (!ui_input_line(ans, sizeof(ans), NULL)) return;
        if (ans[0] != 'y' && ans[0] != 'Y') {
            printf("已取消。\n");
            return;
        }
    }

    if (sys_load(sys, V_FILE_NAME)) {
        printf("✅ 成功读取 %d 条记录，%d 个科目!\n",
               sys->count, sys->subject_count);
    } else {
        printf("读取失败（可能文件不存在或损坏）!\n");
    }
}


/* ---------- 命令 11：导出 CSV ---------- */

void cmd_export(const GradeSystem *sys) {
    ui_title("导出 CSV");
    if (no_data(sys)) return;

    if (sys_export_csv(sys, V_CSV_NAME)) {
        printf("✅ 已导出 %d 条到 %s!\n", sys->count, V_CSV_NAME);
        printf("   用 Excel/WPS 直接打开即可。\n");
    } else {
        printf("导出失败!\n");
    }
}


/* ---------- 命令 12：柱状图 ---------- */

void cmd_chart(const GradeSystem *sys) {
    int top;

    ui_title("排名柱状图");
    if (no_data(sys)) return;

    /* 先按总分排一下，保证顺序对。 */
    sys_sort((GradeSystem *)sys, SORT_SUM, 0);

    top = sys->count >= 10 ? 10 : sys->count;
    ui_bar_chart(sys, top);
}


/* ---------- 命令 13：科目管理 ---------- */

void cmd_subjects(GradeSystem *sys) {
    int i, choice, new_count;
    char buf[32];

    ui_title("管理科目");
    printf("当前 %d 科：\n", sys->subject_count);
    for (i = 0; i < sys->subject_count; i++) {
        printf("  %d. %s\n", i + 1, sys->subjects[i]);
    }
    printf("\n科目数决定表格列数。改了科目后所有成绩按实际数量保留。\n\n");
    printf("  1. 设置科目数（2-%d）\n", V_MAX_SUBJECTS);
    printf("  2. 重命名某个科目\n");
    printf("  0. 返回\n");

    choice = ui_input_int("选择: ");
    switch (choice) {
    case 1:
        new_count = ui_input_int("新的科目数: ");
        if (new_count < 2 || new_count > V_MAX_SUBJECTS) {
            printf("科目数要在 2 到 %d 之间!\n", V_MAX_SUBJECTS);
            return;
        }

        /* 多出来的科目让用户填名字。 */
        if (new_count > sys->subject_count) {
            for (i = sys->subject_count; i < new_count; i++) {
                char prompt[64];
                snprintf(prompt, sizeof(prompt), "第 %d 科名称: ", i + 1);
                if (!ui_input_line(buf, sizeof(buf), prompt) || buf[0] == '\0') {
                    char tmp[32];                              /* 临时 buffer 够大，绕开 GCC 截断告警。 */
                    snprintf(tmp, sizeof(tmp), "科目%d", i + 1);
                    strncpy(sys->subjects[i], tmp, 15);
                    sys->subjects[i][15] = '\0';
                } else {
                    strncpy(sys->subjects[i], buf, 15);
                    sys->subjects[i][15] = '\0';
                }
            }
        }

        /* 减少的话后面的 score 数组位还留着，但不显示了。重算一下。 */
        sys->subject_count = new_count;
        for (i = 0; i < sys->count; i++) {
            sys_recalc(&sys->students[i], sys->subject_count);
        }
        printf("✅ 已更新为 %d 科!\n", sys->subject_count);
        break;

    case 2:
        i = ui_input_int("要改名的科目序号: ");
        if (i < 1 || i > sys->subject_count) {
            printf("序号不对!\n");
            return;
        }
        if (ui_input_line(buf, sizeof(buf), "新名字: ") && buf[0] != '\0') {
            strncpy(sys->subjects[i - 1], buf, 15);
            sys->subjects[i - 1][15] = '\0';
            printf("✅ 已改名!\n");
        }
        break;

    default:
        break;
    }
}
