/*
 * 学生成绩管理系统
 *
 * 控制台小程序，能录入、查询、修改、删除、排序、统计，
 * 数据存到 students.dat，下次运行可以接着用。
 *
 * 编译：gcc main.c -o student
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_STU    100          /* 最多存多少人 */
#define NAME_LEN   32           /* 名字最长多少字节 */
#define SUBJECTS   3            /* 科目数：语文、数学、英语 */
#define PASS_LINE  60.0f        /* 及格线 */
#define DATA_FILE  "students.dat"

typedef struct {
    int   id;                   /* 学号 */
    char  name[NAME_LEN];       /* 姓名 */
    float score[SUBJECTS];      /* 三科成绩 */
    float sum;                  /* 总分 */
    float avg;                  /* 平均分 */
    int   rank;                 /* 名次，0 表示没排过 */
} Student;

static Student stu[MAX_STU];
static int stu_num = 0;

/* ------------------------------------------------------------------ */
/* 输入                                                                */
/* ------------------------------------------------------------------ */

static int input_eof = 0;       /* 读到 Ctrl+Z 或者输入结束时置 1 */

/* 读一行并去掉回车。输入太长的话把剩下的字符吃掉，
 * 不然会冲掉下一次输入。 */
static char *input(char *buf, int size)
{
    int len;
    int ch;

    if (fgets(buf, size, stdin) == NULL) {
        input_eof = 1;
        return NULL;
    }

    len = (int)strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        while ((ch = getchar()) != '\n' && ch != EOF) {
        }
    }
    return buf;
}

/* 读一个整数，输入不是数字就让人重打 */
static int input_int(const char *tip)
{
    char buf[32];
    char *end;
    long v;

    while (1) {
        printf("%s", tip);
        if (input(buf, sizeof(buf)) == NULL) {
            return 0;
        }

        v = strtol(buf, &end, 10);
        if (end != buf && *end == '\0') {
            return (int)v;
        }
        printf("输入的不是整数，重新输入。\n");
    }
}

/* 把字符串转成 0~100 的分数，成功返回 1 */
static int to_score(const char *str, float *out)
{
    char *end;
    double v;

    v = strtod(str, &end);
    if (end == str || *end != '\0') {
        return 0;
    }

    if (v < 0.0) {
        v = 0.0;
    }
    if (v > 100.0) {
        v = 100.0;
    }

    *out = (float)v;
    return 1;
}

/* 读一门课的成绩 */
static float input_score(const char *tip)
{
    char buf[32];
    float v;

    while (1) {
        printf("%s", tip);
        if (input(buf, sizeof(buf)) == NULL) {
            return 0.0f;
        }
        if (to_score(buf, &v)) {
            return v;
        }
        printf("分数要在 0 到 100 之间。\n");
    }
}

/* 读姓名，空的不行 */
static void input_name(char *name)
{
    while (1) {
        printf("姓名: ");
        if (input(name, NAME_LEN) == NULL) {
            name[0] = '\0';
            return;
        }
        if (name[0] != '\0') {
            return;
        }
        printf("姓名不能空着。\n");
    }
}

/* 重新算一遍总分和平均分 */
static void recalc(Student *s)
{
    int i;
    float sum = 0.0f;

    for (i = 0; i < SUBJECTS; i++) {
        sum += s->score[i];
    }
    s->sum = sum;
    s->avg = sum / SUBJECTS;
}

/* ------------------------------------------------------------------ */
/* 输出                                                                */
/* ------------------------------------------------------------------ */

static void line(void)
{
    printf("--------------------------------------------------\n");
}

static void title(const char *text)
{
    printf("\n");
    line();
    printf("%s\n", text);
    line();
}

static void table_head(void)
{
    printf("%-6s %-10s %6s %6s %6s %7s %7s %4s\n",
           "学号", "姓名", "语文", "数学", "英语", "总分", "平均", "名次");
    line();
}

static void table_row(const Student *s)
{
    printf("%-6d %-10s %6.1f %6.1f %6.1f %7.1f %7.1f %4d\n",
           s->id, s->name,
           s->score[0], s->score[1], s->score[2],
           s->sum, s->avg, s->rank);
}

/* ------------------------------------------------------------------ */
/* 数据操作                                                            */
/* ------------------------------------------------------------------ */

/* 按学号找，返回下标，找不到返回 -1 */
static int find(int id)
{
    int i;

    for (i = 0; i < stu_num; i++) {
        if (stu[i].id == id) {
            return i;
        }
    }
    return -1;
}

/* 一条数据都没有的时候提示一下，返回 1 */
static int no_data(void)
{
    if (stu_num == 0) {
        printf("还没有学生记录。\n");
        return 1;
    }
    return 0;
}

static void add_student(void)
{
    Student s;

    title("录入学生");

    if (stu_num >= MAX_STU) {
        printf("最多只能存 %d 个人。\n", MAX_STU);
        return;
    }

    while (1) {
        s.id = input_int("学号: ");
        if (input_eof) {
            return;
        }
        if (s.id <= 0) {
            printf("学号得是正数。\n");
            continue;
        }
        if (find(s.id) >= 0) {
            printf("学号 %d 已经有人用了。\n", s.id);
            continue;
        }
        break;
    }

    input_name(s.name);
    if (s.name[0] == '\0') {
        return;
    }

    s.score[0] = input_score("语文: ");
    s.score[1] = input_score("数学: ");
    s.score[2] = input_score("英语: ");
    recalc(&s);
    s.rank = 0;

    stu[stu_num] = s;
    stu_num++;

    printf("录入成功：%s，总分 %.1f，平均 %.1f。\n", s.name, s.sum, s.avg);
}

static void show_all(void)
{
    int i;

    title("学生成绩表");
    if (no_data()) {
        return;
    }

    printf("共 %d 人\n", stu_num);
    table_head();
    for (i = 0; i < stu_num; i++) {
        table_row(&stu[i]);
    }
}

static void search_id(void)
{
    int id;
    int idx;

    title("按学号查询");
    if (no_data()) {
        return;
    }

    id = input_int("学号: ");
    if (input_eof) {
        return;
    }

    idx = find(id);
    if (idx < 0) {
        printf("查不到学号 %d。\n", id);
        return;
    }

    table_head();
    table_row(&stu[idx]);
}

static void search_name(void)
{
    char key[NAME_LEN];
    int i;
    int found = 0;

    title("按姓名查询");
    if (no_data()) {
        return;
    }

    printf("姓名关键字: ");
    if (input(key, NAME_LEN) == NULL || key[0] == '\0') {
        printf("没输入关键字。\n");
        return;
    }

    table_head();
    for (i = 0; i < stu_num; i++) {
        if (strstr(stu[i].name, key) != NULL) {
            table_row(&stu[i]);
            found++;
        }
    }

    if (found == 0) {
        printf("没有名字带 \"%s\" 的学生。\n", key);
    } else {
        printf("找到 %d 条。\n", found);
    }
}

/* 改一科成绩，直接回车就不改 */
static void edit_score(const char *subject, float old, float *p)
{
    char buf[32];
    float v;

    printf("%s（原值 %.1f，回车不改）: ", subject, old);
    if (input(buf, sizeof(buf)) == NULL) {
        return;
    }
    if (buf[0] == '\0') {
        return;
    }

    if (to_score(buf, &v)) {
        *p = v;
    } else {
        printf("看不明白，%s保持原值。\n", subject);
    }
}

static void modify_student(void)
{
    int id;
    int idx;
    char name[NAME_LEN];

    title("修改信息");
    if (no_data()) {
        return;
    }

    id = input_int("要改的学号: ");
    if (input_eof) {
        return;
    }

    idx = find(id);
    if (idx < 0) {
        printf("没有学号 %d 的学生。\n", id);
        return;
    }

    printf("\n原来的信息：\n");
    table_head();
    table_row(&stu[idx]);

    printf("\n直接回车表示不改。\n");
    printf("姓名（原 %s）: ", stu[idx].name);
    if (input(name, NAME_LEN) == NULL) {
        return;
    }
    if (name[0] != '\0') {
        strncpy(stu[idx].name, name, NAME_LEN - 1);
        stu[idx].name[NAME_LEN - 1] = '\0';
    }

    edit_score("语文", stu[idx].score[0], &stu[idx].score[0]);
    edit_score("数学", stu[idx].score[1], &stu[idx].score[1]);
    edit_score("英语", stu[idx].score[2], &stu[idx].score[2]);

    recalc(&stu[idx]);

    printf("\n改完了：\n");
    table_head();
    table_row(&stu[idx]);
}

static void delete_student(void)
{
    int id;
    int idx;
    int i;
    char c[8];

    title("删除学生");
    if (no_data()) {
        return;
    }

    id = input_int("要删的学号: ");
    if (input_eof) {
        return;
    }

    idx = find(id);
    if (idx < 0) {
        printf("没有学号 %d 的学生。\n", id);
        return;
    }

    printf("要删掉 %s（学号 %d），确定吗？(y/N): ", stu[idx].name, id);
    if (input(c, sizeof(c)) == NULL) {
        return;
    }
    if (c[0] != 'y' && c[0] != 'Y') {
        printf("算了，没删。\n");
        return;
    }

    /* 后面的人往前挪一格 */
    for (i = idx; i < stu_num - 1; i++) {
        stu[i] = stu[i + 1];
    }
    stu_num--;

    printf("已删除，还剩 %d 条。\n", stu_num);
}

/* ------------------------------------------------------------------ */
/* 排序                                                                */
/* ------------------------------------------------------------------ */

static int cmp_sum(const void *a, const void *b)
{
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;

    if (y->sum > x->sum) {
        return 1;
    }
    if (y->sum < x->sum) {
        return -1;
    }
    return x->id - y->id;
}

static int cmp_avg(const void *a, const void *b)
{
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;

    if (y->avg > x->avg) {
        return 1;
    }
    if (y->avg < x->avg) {
        return -1;
    }
    return x->id - y->id;
}

static int cmp_id(const void *a, const void *b)
{
    const Student *x = (const Student *)a;
    const Student *y = (const Student *)b;

    return x->id - y->id;
}

static void sort_menu(void)
{
    char c[8];
    int i;

    title("排序");
    if (no_data()) {
        return;
    }

    printf("1. 总分从高到低\n");
    printf("2. 平均分从高到低\n");
    printf("3. 学号从小到大\n");
    printf("选择: ");
    if (input(c, sizeof(c)) == NULL) {
        return;
    }

    switch (c[0]) {
    case '1':
        qsort(stu, stu_num, sizeof(Student), cmp_sum);
        /* 总分一样的名次也一样，后面的名次跳着排 */
        for (i = 0; i < stu_num; i++) {
            if (i > 0 && stu[i].sum == stu[i - 1].sum) {
                stu[i].rank = stu[i - 1].rank;
            } else {
                stu[i].rank = i + 1;
            }
        }
        break;
    case '2':
        qsort(stu, stu_num, sizeof(Student), cmp_avg);
        break;
    case '3':
        qsort(stu, stu_num, sizeof(Student), cmp_id);
        for (i = 0; i < stu_num; i++) {
            stu[i].rank = 0;
        }
        break;
    default:
        printf("没有这个选项。\n");
        return;
    }

    show_all();
}

/* ------------------------------------------------------------------ */
/* 统计                                                                */
/* ------------------------------------------------------------------ */

static void show_stat(void)
{
    const char *sub[SUBJECTS] = {"语文", "数学", "英语"};
    float total[SUBJECTS] = {0.0f, 0.0f, 0.0f};
    float high[SUBJECTS] = {0.0f, 0.0f, 0.0f};
    float low[SUBJECTS] = {100.0f, 100.0f, 100.0f};
    int   pass[SUBJECTS] = {0, 0, 0};
    int   all_pass = 0;
    float sum_all = 0.0f;
    float max_all = 0.0f;
    float min_all = 0.0f;
    int   i;
    int   j;

    title("统计分析");
    if (no_data()) {
        return;
    }

    for (i = 0; i < stu_num; i++) {
        int ok = 1;

        for (j = 0; j < SUBJECTS; j++) {
            float v = stu[i].score[j];

            total[j] += v;
            if (v > high[j]) {
                high[j] = v;
            }
            if (v < low[j]) {
                low[j] = v;
            }
            if (v >= PASS_LINE) {
                pass[j]++;
            } else {
                ok = 0;
            }
        }
        if (ok) {
            all_pass++;
        }

        sum_all += stu[i].sum;
        if (i == 0 || stu[i].sum > max_all) {
            max_all = stu[i].sum;
        }
        if (i == 0 || stu[i].sum < min_all) {
            min_all = stu[i].sum;
        }
    }

    for (j = 0; j < SUBJECTS; j++) {
        printf("%s：平均 %.1f，最高 %.1f，最低 %.1f，及格 %d 人（%.1f%%）\n",
               sub[j], total[j] / stu_num, high[j], low[j],
               pass[j], pass[j] * 100.0f / stu_num);
    }

    line();
    printf("总人数 %d，三科全及格 %d 人\n", stu_num, all_pass);
    printf("总分平均 %.1f，最高 %.1f，最低 %.1f\n",
           sum_all / stu_num, max_all, min_all);
}

/* ------------------------------------------------------------------ */
/* 文件                                                                */
/* ------------------------------------------------------------------ */

static void save_file(void)
{
    FILE *fp;
    int n;

    title("保存数据");

    fp = fopen(DATA_FILE, "wb");
    if (fp == NULL) {
        printf("打不开 %s，检查一下权限或者磁盘。\n", DATA_FILE);
        return;
    }

    /* 先写人数，再写每个人的数据 */
    if (fwrite(&stu_num, sizeof(int), 1, fp) != 1) {
        printf("写入失败。\n");
        fclose(fp);
        return;
    }

    if (stu_num > 0) {
        n = (int)fwrite(stu, sizeof(Student), stu_num, fp);
        if (n != stu_num) {
            printf("只写进去 %d 条，本来该有 %d 条。\n", n, stu_num);
            fclose(fp);
            return;
        }
    }

    fclose(fp);
    printf("已保存 %d 条到 %s。\n", stu_num, DATA_FILE);
}

static void load_file(void)
{
    FILE *fp;
    int n;
    int i;
    char c[8];

    title("读取数据");

    fp = fopen(DATA_FILE, "rb");
    if (fp == NULL) {
        printf("没找到 %s。\n", DATA_FILE);
        return;
    }

    if (fread(&n, sizeof(int), 1, fp) != 1) {
        printf("文件头读不出来，可能坏了。\n");
        fclose(fp);
        return;
    }

    if (n < 0 || n > MAX_STU) {
        printf("文件里的人数 %d 不对劲，可能坏了。\n", n);
        fclose(fp);
        return;
    }

    if (stu_num > 0) {
        printf("现在有 %d 条，读进来会覆盖掉，确定吗？(y/N): ", stu_num);
        if (input(c, sizeof(c)) == NULL) {
            fclose(fp);
            return;
        }
        if (c[0] != 'y' && c[0] != 'Y') {
            printf("没读。\n");
            fclose(fp);
            return;
        }
    }

    if (n > 0) {
        if ((int)fread(stu, sizeof(Student), n, fp) != n) {
            printf("数据没读完，文件可能坏了。\n");
            fclose(fp);
            return;
        }
    }

    stu_num = n;
    fclose(fp);

    /* 文件里的总分、平均分可能是旧的，重算一遍 */
    for (i = 0; i < stu_num; i++) {
        recalc(&stu[i]);
    }
    printf("读进来 %d 条。\n", stu_num);
}

/* ------------------------------------------------------------------ */
/* 菜单                                                                */
/* ------------------------------------------------------------------ */

static void menu(void)
{
    printf("\n");
    printf("==========================================\n");
    printf("         学生成绩管理系统   %d 人\n", stu_num);
    printf("==========================================\n");
    printf("  1. 录入学生\n");
    printf("  2. 显示全部\n");
    printf("  3. 按学号查\n");
    printf("  4. 按姓名查\n");
    printf("  5. 修改信息\n");
    printf("  6. 删除学生\n");
    printf("  7. 排序\n");
    printf("  8. 统计\n");
    printf("  9. 保存到文件\n");
    printf(" 10. 从文件读取\n");
    printf("  0. 退出\n");
    printf("==========================================\n");
}

/* 停一下，让人看清输出 */
static void wait_key(void)
{
    printf("\n按回车继续...");
    (void)getchar();
}

static void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}

int main(void)
{
    int choice;

    while (1) {
        clear_screen();
        menu();
        choice = input_int("请选择: ");

        if (choice == 0) {
            break;
        }

        switch (choice) {
        case 1:  add_student();     break;
        case 2:  show_all();        break;
        case 3:  search_id();       break;
        case 4:  search_name();     break;
        case 5:  modify_student();  break;
        case 6:  delete_student();  break;
        case 7:  sort_menu();       break;
        case 8:  show_stat();       break;
        case 9:  save_file();       break;
        case 10: load_file();       break;
        default: printf("没有这个选项。\n"); break;
        }

        if (input_eof) {
            break;
        }
        wait_key();
    }

    if (stu_num > 0) {
        char c[8];

        printf("\n要不要保存一下？(Y/n): ");
        if (input(c, sizeof(c)) != NULL && c[0] != 'n' && c[0] != 'N') {
            save_file();
        }
    }

    printf("再见。\n");
    return 0;
}
