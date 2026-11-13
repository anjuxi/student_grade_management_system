/*
 * student.h —— 学生成绩管理系统共用的头文件。
 *
 * 把几个 .c 文件里都要用的东西丢在这里：
 * Student 结构体、GradeSystem 上下文、以及函数声明。
 * 其他模块 include 它就行，不用互相 extern。
 */

#ifndef STUDENT_H
#define STUDENT_H

#include <stdio.h>

/* 几个基本常量。 */
#define V_NAME_LEN      32          /* 名字最多 31 个字符，留一位 '\0'。 */
#define V_DEFAULT_CAP   100         /* 初始容量。不够了会自动翻倍，所以不用慌。 */
#define V_PASS_LINE     60.0f       /* 及格线，低于这个算不及格。 */
#define V_FILE_NAME     "students.dat"       /* 存档文件。 */
#define V_CSV_NAME      "students.csv"       /* CSV 导出文件。 */
#define V_MAX_SUBJECTS  10          /* 最多允许 10 科，防止数组越界。 */


/* 一条学生记录。
 *
 * 所有字段都是裸的，因为要直接 fwrite 成二进制。
 * 改这个结构体之前想清楚——改完旧的 dat 文件就读不进来了。
 */
typedef struct {
    int     id;                         /* 学号，正整数，不能重复。 */
    char    name[V_NAME_LEN];           /* 姓名。 */
    float   score[V_MAX_SUBJECTS];      /* 各科成绩，下标跟 subjects 对应。 */
    float   sum;                        /* 总分，别手动赋值，调 recalc 自动算。 */
    float   avg;                        /* 平均分，同上。 */
    int     rank;                       /* 排名。0 表示还没排过。 */
} Student;


/* 整个系统的上下文。把原来散落在各文件的全局变量收进一个 struct。 */
typedef struct {
    Student *students;                  /* 动态数组。 */
    int     count;                      /* 当前有多少人。 */
    int     capacity;                   /* 数组能装多少人，不够会自动扩。 */
    int     subject_count;              /* 当前启用几科。 */
    char    subjects[V_MAX_SUBJECTS][16];   /* 科目名，每项最长 15 字。 */
} GradeSystem;


/* 排序类型，sys_sort 的 type 参数用。 */
enum {
    SORT_SUM = 1,       /* 总分从高到低。 */
    SORT_AVG  = 2,      /* 平均分从高到低。 */
    SORT_ID   = 3,      /* 学号从小到大。 */
    SORT_SUBJECT = 4    /* 指定科目降序，subject_idx 才有用。 */
};


/* student.c 里的函数。 */
void    sys_init(GradeSystem *sys);                 /* 初始化，分配内存，设默认三科。 */
void    sys_free(GradeSystem *sys);                 /* 释放内存，退出前调。 */
int     sys_add(GradeSystem *sys, const Student *s);/* 加一个学生，返回 1 成功 0 失败。 */
int     sys_find_by_id(const GradeSystem *sys, int id); /* 按学号找，返回下标，没找到 -1。 */
void    sys_remove_at(GradeSystem *sys, int idx);    /* 删第 idx 个，后面往前挪。 */
void    sys_recalc(Student *s, int subject_count);  /* 重算总分和平均分。改成绩后要调这个。 */
void    sys_sort(GradeSystem *sys, int type, int subject_idx); /* 排序。subject_idx 只在按单科时用。 */
void    sys_rank(GradeSystem *sys);                 /* 按总分排名，填 rank 字段。 */
const char *grade_level(float avg);                  /* 根据平均分返回等级：优秀/良好/中等/及格/不及格。 */
int    *cmp_arg_ptr(void);                           /* 给按单科排序的 qsort 比较器传参数用的，内部函数。 */


/* file_io.c 里的函数。 */
int     sys_save(const GradeSystem *sys, const char *path); /* 存成二进制，返回 1 成功。 */
int     sys_load(GradeSystem *sys, const char *path);       /* 读进来，覆盖现有数据。 */
int     sys_export_csv(const GradeSystem *sys, const char *path); /* 导出 CSV，Excel 直接开。 */


/* ui.c 里的函数。 */
void    ui_line(void);                                              /* 打印一条分隔线。 */
void    ui_title(const char *text);                                 /* 带分隔线的标题。 */
void    ui_table_head(int subject_count, const char subjects[][16]); /* 表头。 */
void    ui_table_row(const Student *s, int subject_count);          /* 一行。 */
void    ui_bar_chart(const GradeSystem *sys, int top_n);            /* ASCII 柱状图。 */
int     ui_menu(const GradeSystem *sys);                            /* 菜单，返回用户选的号。 */
int     ui_input_line(char *buf, int size, const char *prompt);     /* 读一行字符串。 */
int     ui_input_int(const char *prompt);                           /* 读整数，不合法让重打。 */
float   ui_input_score(const char *prompt);                         /* 读分数，自动 clamp 到 0~100。 */
void    ui_input_name(char *name);                                  /* 读姓名，不能为空。 */
void    ui_pause(void);                                             /* 按回车继续。 */
int     ui_has_eof(void);                                           /* 检查 Ctrl+Z/D 中断了没。 */


/* commands.c 里的函数。每个对应菜单里的一项。 */
void    cmd_add(GradeSystem *sys);          /* 录入。 */
void    cmd_show_all(const GradeSystem *sys); /* 显示全部。 */
void    cmd_search_id(const GradeSystem *sys); /* 按学号查。 */
void    cmd_search_name(const GradeSystem *sys); /* 按姓名模糊查。 */
void    cmd_modify(GradeSystem *sys);       /* 修改。 */
void    cmd_delete(GradeSystem *sys);       /* 删除，带确认。 */
void    cmd_sort(GradeSystem *sys);         /* 排序子菜单。 */
void    cmd_stats(const GradeSystem *sys);  /* 统计分析。 */
void    cmd_save(const GradeSystem *sys);   /* 保存。 */
void    cmd_load(GradeSystem *sys);         /* 加载。 */
void    cmd_export(const GradeSystem *sys); /* 导出 CSV。 */
void    cmd_chart(const GradeSystem *sys);  /* 柱状图。 */
void    cmd_subjects(GradeSystem *sys);     /* 管理科目。 */

#endif /* STUDENT_H */
