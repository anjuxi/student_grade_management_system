/*
 * main.c —— 程序入口和主循环。
 *
 * 就是套个 while(1) 循环，读用户选哪个菜单，分派给对应 cmd_*。
 * 退出时提示要不要保存，然后 free 干净再见。
 */

#include "student.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* 清屏。Windows 用 cls，别的终端用 ANSI 转义序列。 */
static void clear_screen(void) {
#ifdef _WIN32
    system("cls");
#else
    printf("\033[2J\033[H");
#endif
}


int main(void) {
    GradeSystem sys;
    int choice;

    sys_init(&sys);
    printf("学生成绩管理系统启动完成!\n");
    printf("  初始容量：%d 人，默认 %d 科。\n",
           sys.capacity, sys.subject_count);

    while (1) {
        clear_screen();
        choice = ui_menu(&sys);

        if (choice == 0) {
            break;
        }

        switch (choice) {
        case 1:    cmd_add(&sys);          break;
        case 2:    cmd_show_all(&sys);     break;
        case 3:    cmd_search_id(&sys);    break;
        case 4:    cmd_search_name(&sys);  break;
        case 5:    cmd_modify(&sys);       break;
        case 6:    cmd_delete(&sys);       break;
        case 7:    cmd_sort(&sys);         break;
        case 8:    cmd_stats(&sys);        break;
        case 9:    cmd_save(&sys);         break;
        case 10:   cmd_load(&sys);         break;
        case 11:   cmd_export(&sys);       break;
        case 12:   cmd_chart(&sys);        break;
        case 13:   cmd_subjects(&sys);     break;
        default:   printf("没有这个选项，请输入 0-13!\n"); break;
        }

        /* Ctrl+Z / Ctrl+D 也当作退出。 */
        if (ui_has_eof()) {
            printf("\n检测到输入流结束，准备退出...\n");
            break;
        }

        ui_pause();
    }

    /* 退出前问一下要不要保存。 */
    if (sys.count > 0) {
        char ans[8];
        printf("\n要不要保存一下？(Y/n): ");
        if (ui_input_line(ans, sizeof(ans), NULL)) {
            if (ans[0] != 'n' && ans[0] != 'N') {
                cmd_save(&sys);
            }
        }
    }

    sys_free(&sys);
    printf("\n再见!\n");
    return 0;
}
