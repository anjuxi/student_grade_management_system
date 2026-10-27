# Student Grade Management System

A small console program written in C for managing student records: grades, ranking
and a few statistics. Everything is kept in one source file, and the data is saved
to a binary file so you can continue where you left off.

## Features

- Add a student (ID, name, three subject scores)
- Show the whole list in a table
- Search by student ID, or by part of a name
- Edit an existing record; press Enter to keep a field unchanged
- Delete a record, with a confirmation prompt
- Sort by total score, by average, or by ID
- Statistics: average / highest / lowest per subject, pass count and pass rate
- Save to and load from `students.dat`
- Screen is cleared before every menu, so the output does not pile up

## Build

Any C89 compiler will do. With GCC:

```
gcc main.c -o student
```

On Windows use `gcc main.c -o student.exe`. For Dev-C++ just open `main.c` and
press F11. With MSVC:

```
cl main.c
```

## Usage

Run the executable and pick a number from the menu:

```
==========================================
         学生成绩管理系统   0 人
==========================================
  1. 录入学生
  2. 显示全部
  3. 按学号查
  4. 按姓名查
  5. 修改信息
  6. 删除学生
  7. 排序
  8. 统计
  9. 保存到文件
 10. 从文件读取
  0. 退出
```

Note: the interface text is in Chinese, since the program was written for a
Chinese school setting. The numbers on the left are what you type.

A normal session looks like this: load the saved file first (option 10), then
add or edit records, and save before leaving (option 9, or answer `y` when the
program asks on exit).

Input rules:

- Student IDs must be positive integers, and no two records may share one.
- Scores are clamped to the 0–100 range.
- Pressing Enter without typing anything keeps the old value while editing.

## Data file

`students.dat` is a binary file. It starts with one `int` holding the number of
records, followed by that many `Student` structs:

```c
typedef struct {
    int   id;
    char  name[32];
    float score[3];     /* Chinese, Maths, English */
    float sum;
    float avg;
    int   rank;
} Student;
```

Because the struct is written as raw memory, the file is tied to the layout and
endianness of the compiler that wrote it. It is fine for moving data between runs
of the same build, but do not treat it as a portable format. Delete the file if
you want to start from scratch.

## Limits

- At most 100 students, stored in a fixed array.
- Three subjects are hard-coded (see `SUBJECTS` and the `score` array).
- No password or user accounts - it is meant to run locally by one person.
