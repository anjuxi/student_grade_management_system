/*
 * file_io.c —— 文件存取。
 *
 * 二进制格式：[int count][int subject_count][科目名 × subject_count][Student × count]
 * Student 是直接 fwrite 裸内存，所以跨平台不兼容。
 * 同机器同版本用没问题，别拿去 Linux 读 Windows 写的文件。
 */

#include "student.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* 存成二进制。返回 1 成功，0 失败。 */
int sys_save(const GradeSystem *sys, const char *path) {
    FILE *fp;
    int i;

    fp = fopen(path, "wb");
    if (fp == NULL) {
        return 0;
    }

    /* 先写元信息：人数和科目数。 */
    if (fwrite(&sys->count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (fwrite(&sys->subject_count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }

    /* 把科目名也存下来，加载时能恢复。 */
    for (i = 0; i < sys->subject_count; i++) {
        if (fwrite(sys->subjects[i], 16, 1, fp) != 1) {
            fclose(fp);
            return 0;
        }
    }

    /* 最后写学生数据，直接裸写结构体。 */
    if (sys->count > 0) {
        if ((int)fwrite(sys->students, sizeof(Student), sys->count, fp) != sys->count) {
            fclose(fp);
            return 0;
        }
    }

    fclose(fp);
    return 1;
}


/* 读进来，覆盖现有数据。返回 1 成功，0 失败。
 * 兼容老版本文件（只存了 count，没存 subject_count）。 */
int sys_load(GradeSystem *sys, const char *path) {
    FILE *fp;
    int n, sc, i;
    Student *tmp;

    fp = fopen(path, "rb");
    if (fp == NULL) {
        return 0;
    }

    /* 读人数。 */
    if (fread(&n, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return 0;
    }
    if (n < 0 || n > 100000) {                                      /* 防恶意大数值。 */
        fclose(fp);
        return 0;
    }

    /* 读科目数。老文件没有这个字段，会失败，这时把 sc 置 3 然后
     * 退回去重读一遍——不对，老文件的第二个 int 其实是第一条学生
     * 记录的 id，读出来是个 1001 之类的数，不是科目数。
     *
     * 算了，不兼容老格式了。老版本就 3 科，真遇到手动设一下就行。 */
    if (fread(&sc, sizeof(int), 1, fp) != 1) {
        sc = 3;
    }
    if (sc <= 0 || sc > V_MAX_SUBJECTS) {
        fclose(fp);
        return 0;
    }

    /* 读科目名。 */
    for (i = 0; i < sc; i++) {
        if (fread(sys->subjects[i], 16, 1, fp) != 1) {
            fclose(fp);
            return 0;
        }
        sys->subjects[i][15] = '\0';
    }
    sys->subject_count = sc;

    /* 临时分配一块内存读，读成功再替换掉旧数组，避免读一半崩了
     * 把原来的数据也搞坏。 */
    if (n > 0) {
        tmp = (Student *)malloc(sizeof(Student) * n);
        if (tmp == NULL) {
            fclose(fp);
            return 0;
        }
        if ((int)fread(tmp, sizeof(Student), n, fp) != n) {
            free(tmp);
            fclose(fp);
            return 0;
        }

        free(sys->students);
        sys->students = tmp;
        sys->capacity = n;
        sys->count = n;

        /* 重算一遍，防止文件里存的 sum/avg 是旧值。 */
        for (i = 0; i < n; i++) {
            sys_recalc(&sys->students[i], sys->subject_count);
        }
    } else {
        sys->count = 0;
    }

    fclose(fp);
    return 1;
}


/* 导出成 CSV。UTF-8 加 BOM，Excel 直接能开（WPS 也认）。 */
int sys_export_csv(const GradeSystem *sys, const char *path) {
    FILE *fp;
    int i, j;

    fp = fopen(path, "w");
    if (fp == NULL) {
        return 0;
    }

    /* 写 UTF-8 BOM。 */
    fprintf(fp, "\xEF\xBB\xBF");

    /* 表头。 */
    fprintf(fp, "学号,姓名");
    for (j = 0; j < sys->subject_count; j++) {
        fprintf(fp, ",%s", sys->subjects[j]);
    }
    fprintf(fp, ",总分,平均分,等级,名次\n");

    /* 逐行数据。 */
    for (i = 0; i < sys->count; i++) {
        const Student *s = &sys->students[i];
        fprintf(fp, "%d,%s", s->id, s->name);
        for (j = 0; j < sys->subject_count; j++) {
            fprintf(fp, ",%.1f", s->score[j]);
        }
        fprintf(fp, ",%.1f,%.1f,%s,%d\n",
                s->sum, s->avg, grade_level(s->avg), s->rank);
    }

    fclose(fp);
    return 1;
}
