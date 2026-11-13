/*
 * student.c —— 增删改查、排序排名、成绩等级这些核心逻辑。
 *
 * 这些函数都是纯逻辑，不直接 printf。
 * 要输出东西走 ui.c，要存文件走 file_io.c，别搞混。
 */

#include "student.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* 初始化。先按默认容量分配一块内存，默认三科：语数英。 */
void sys_init(GradeSystem *sys) {
    int i;

    sys->count = 0;                                                 /* 一开始没人。 */
    sys->capacity = V_DEFAULT_CAP;
    sys->students = (Student *)malloc(sizeof(Student) * sys->capacity);
    if (sys->students == NULL) {                                    /* 内存不够直接崩。 */
        fprintf(stderr, "malloc 失败，程序跑不动。\n");
        exit(1);
    }

    sys->subject_count = 3;
    strncpy(sys->subjects[0], "语文", 15);
    strncpy(sys->subjects[1], "数学", 15);
    strncpy(sys->subjects[2], "英语", 15);

    /* 手工把尾零补上，strncpy 不一定会加 '\0'。 */
    for (i = 0; i < sys->subject_count; i++) {
        sys->subjects[i][15] = '\0';
    }
}


/* 释放。退出前调一下，好习惯。 */
void sys_free(GradeSystem *sys) {
    free(sys->students);
    sys->students = NULL;
    sys->count = 0;
    sys->capacity = 0;
}


/* 内部用：不够装了就翻倍扩容。
 * 倍增法摊销 O(1)，比每次加 10 靠谱。
 * 返回 1 成功，0 失败。 */
static int sys_reserve(GradeSystem *sys, int needed) {
    Student *p;
    int new_cap;

    if (needed <= sys->capacity) {
        return 1;                                                   /* 容量够，不用动。 */
    }

    new_cap = sys->capacity == 0 ? 8 : sys->capacity;
    while (new_cap < needed) {
        new_cap *= 2;
    }

    p = (Student *)realloc(sys->students, sizeof(Student) * new_cap);
    if (p == NULL) {
        return 0;
    }

    sys->students = p;
    sys->capacity = new_cap;
    return 1;
}


/* 往系统里加一个学生。返回 1 成功，0 失败（学号重复或内存不够）。 */
int sys_add(GradeSystem *sys, const Student *s) {
    if (sys_find_by_id(sys, s->id) >= 0) {
        return 0;                                                   /* 学号已存在。 */
    }
    if (!sys_reserve(sys, sys->count + 1)) {
        return 0;                                                   /* 扩内存失败。 */
    }
    sys->students[sys->count] = *s;                                /* 结构体直接拷贝。 */
    sys->count++;
    return 1;
}


/* 按学号查，返回下标。找不到返回 -1。
 * 数据量小，线性扫就够了，犯不着搞哈希表。 */
int sys_find_by_id(const GradeSystem *sys, int id) {
    int i;
    for (i = 0; i < sys->count; i++) {
        if (sys->students[i].id == id) {
            return i;
        }
    }
    return -1;
}


/* 删第 idx 条。后面的整体左移覆盖。 */
void sys_remove_at(GradeSystem *sys, int idx) {
    int i;
    if (idx < 0 || idx >= sys->count) {
        return;
    }
    for (i = idx; i < sys->count - 1; i++) {
        sys->students[i] = sys->students[i + 1];
    }
    sys->count--;
}


/* 改了成绩必须调这个，让总分、平均分跟上。 */
void sys_recalc(Student *s, int subject_count) {
    int i;
    float sum = 0.0f;

    for (i = 0; i < subject_count; i++) {
        sum += s->score[i];
    }
    s->sum = sum;
    s->avg = subject_count > 0 ? sum / subject_count : 0.0f;       /* 防除零。 */
}


/* 几个 qsort 比较器。返回值约定：负数 a 在前，正数 b 在前，0 相同。 */

/* 总分降序。总分相同学号小的排前面。 */
static int cmp_sum_desc(const void *a, const void *b) {
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;
    if (y->sum > x->sum) return 1;
    if (y->sum < x->sum) return -1;
    return x->id - y->id;
}

/* 平均分降序。 */
static int cmp_avg_desc(const void *a, const void *b) {
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;
    if (y->avg > x->avg) return 1;
    if (y->avg < x->avg) return -1;
    return x->id - y->id;
}

/* 学号升序。 */
static int cmp_id_asc(const void *a, const void *b) {
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;
    return x->id - y->id;
}


/* 按指定科目降序的比较器。
 * C 的 qsort 比较器不能带状态参数，所以用一个文件级变量临时存科目下标。
 * sort_menu 里先设好这个变量，再调 qsort。这是 C 里的惯用手法。 */
static int cmp_subject_idx = 0;

/* 让外部能访问这个变量的地址——cmp_by_subject_desc 要读它。 */
int *cmp_arg_ptr(void) { return &cmp_subject_idx; }

static int cmp_by_subject_desc(const void *a, const void *b) {
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;
    if (y->score[cmp_subject_idx] > x->score[cmp_subject_idx]) return 1;
    if (y->score[cmp_subject_idx] < x->score[cmp_subject_idx]) return -1;
    return x->id - y->id;
}


/* 统一的排名计算：当前数组顺序填 rank，并列的名次相同。
 * 例：95, 95, 90, 85 -> 1, 1, 3, 4
 * 注意：这不是"并列不跳号"（体育那种），而是相同分数同排名，
 * 下一个不同分数跳过中间号。 */
static void fill_rank(GradeSystem *sys) {
    int i;
    for (i = 0; i < sys->count; i++) {
        if (i > 0 && sys->students[i].sum == sys->students[i - 1].sum) {
            sys->students[i].rank = sys->students[i - 1].rank;   /* 和前一个一样。 */
        } else {
            sys->students[i].rank = i + 1;                        /* 新名次。 */
        }
    }
}


/* 对外暴露的排名。先按总分排，再填 rank。 */
void sys_rank(GradeSystem *sys) {
    if (sys->count <= 0) return;
    qsort(sys->students, sys->count, sizeof(Student), cmp_sum_desc);
    fill_rank(sys);
}


/* 排序。type 是上面 SORT_* 枚举值。subject_idx 只在 SORT_SUBJECT 时用。 */
void sys_sort(GradeSystem *sys, int type, int subject_idx) {
    if (sys->count <= 0) return;

    switch (type) {
    case SORT_SUM:
        qsort(sys->students, sys->count, sizeof(Student), cmp_sum_desc);
        fill_rank(sys);                                            /* 排序完必须填 rank。 */
        break;
    case SORT_AVG:
        qsort(sys->students, sys->count, sizeof(Student), cmp_avg_desc);
        fill_rank(sys);                                            /* 之前这里漏了 fill_rank，是 bug！ */
        break;
    case SORT_ID:
        qsort(sys->students, sys->count, sizeof(Student), cmp_id_asc);
        /* 按学号排不代表成绩排名，rank 清零表示"未排名"。 */
        {
            int i;
            for (i = 0; i < sys->count; i++) {
                sys->students[i].rank = 0;
            }
        }
        break;
    case SORT_SUBJECT:
        cmp_subject_idx = subject_idx;                             /* 告诉比较器按哪科排。 */
        qsort(sys->students, sys->count, sizeof(Student), cmp_by_subject_desc);
        fill_rank(sys);
        break;
    default:
        break;
    }
}


/* 纯函数：根据平均分返回等级。方便测试。 */
const char *grade_level(float avg) {
    if (avg >= 90.0f) return "优秀";
    if (avg >= 80.0f) return "良好";
    if (avg >= 70.0f) return "中等";
    if (avg >= V_PASS_LINE) return "及格";
    return "不及格";
}
