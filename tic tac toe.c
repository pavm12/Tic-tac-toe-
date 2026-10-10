/*
 * Крестики-нолики в консоли 
 * Режимы: игрок против бота, игрок против игрока.
 * Поле произвольного прямоугольного размера, длина выигрышной линии настраивается.
 *
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define MIN_DIM 3
#define MAX_DIM 25
#define EMPTY   ' '
#define BUF_LEN 128

typedef struct {
    int   rows;
    int   cols;
    int   win_len;   /* сколько символов подряд нужно для победы */
    int   moves;     /* сколько клеток уже занято */
    char *cells;     /* rows * cols символов, хранится построчно */
} Board;

typedef enum { MODE_BOT = 1, MODE_HUMAN = 2 } GameMode;

typedef enum { RESULT_WIN, RESULT_DRAW, RESULT_EOF } GameResult;

/* ---------- ввод ---------- */

/* Читает строку целиком. Возвращает 0 при EOF. Лишние символы длинной строки отбрасываются. */
static int read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL)
        return 0;

    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    } else {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
    return 1;
}

/* Разбирает целое число из начала строки; *end указывает на остаток. */
static int parse_int(const char *s, int *out, const char **end)
{
    char *e;
    errno = 0;
    long v = strtol(s, &e, 10);
    if (e == s || errno == ERANGE || v < INT_MIN || v > INT_MAX)
        return 0;
    *out = (int)v;
    *end = e;
    return 1;
}

static int only_spaces(const char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\r')
        s++;
    return *s == '\0';
}

/* Запрашивает число в диапазоне [lo, hi]. Возвращает 0 при EOF. */
static int ask_int(const char *prompt, int lo, int hi, int *out)
{
    char buf[BUF_LEN];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;

        int v;
        const char *rest;
        if (parse_int(buf, &v, &rest) && only_spaces(rest) && v >= lo && v <= hi) {
            *out = v;
            return 1;
        }
        printf("Ошибка: введите целое число от %d до %d.\n", lo, hi);
    }
}

/* Вопрос "да/нет". Возвращает 1 (да), 0 (нет), -1 (EOF). */
static int ask_yes_no(const char *prompt)
{
    char buf[BUF_LEN];

    for (;;) {
        printf("%s", prompt);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return -1;

        /* убираем пробелы по краям */
        char *p = buf;
        while (*p == ' ' || *p == '\t')
            p++;
        size_t n = strlen(p);
        while (n > 0 && (p[n - 1] == ' ' || p[n - 1] == '\t' || p[n - 1] == '\r'))
            p[--n] = '\0';

        if (!strcmp(p, "д") || !strcmp(p, "Д") || !strcmp(p, "да") || !strcmp(p, "Да") ||
            !strcmp(p, "y") || !strcmp(p, "Y") || !strcmp(p, "yes") || !strcmp(p, "1"))
            return 1;
        if (!strcmp(p, "н") || !strcmp(p, "Н") || !strcmp(p, "нет") || !strcmp(p, "Нет") ||
            !strcmp(p, "n") || !strcmp(p, "N") || !strcmp(p, "no") || !strcmp(p, "0"))
            return 0;

        printf("Ответьте «д» (да) или «н» (нет).\n");
    }
}

/* ---------- поле ---------- */

static int board_init(Board *b, int rows, int cols, int win_len)
{
    b->rows = rows;
    b->cols = cols;
    b->win_len = win_len;
    b->moves = 0;
    b->cells = (char *)malloc((size_t)rows * (size_t)cols);
    if (b->cells == NULL)
        return 0;
    memset(b->cells, EMPTY, (size_t)rows * (size_t)cols);
    return 1;
}

static void board_free(Board *b)
{
    free(b->cells);
    b->cells = NULL;
}

static char board_get(const Board *b, int r, int c)
{
    return b->cells[r * b->cols + c];
}

static void board_set(Board *b, int r, int c, char sym)
{
    b->cells[r * b->cols + c] = sym;
    b->moves++;
}

static int board_full(const Board *b)
{
    return b->moves == b->rows * b->cols;
}

static int digits(int n)
{
    int d = 1;
    while (n >= 10) {
        n /= 10;
        d++;
    }
    return d;
}

/* Печатает строку s по центру поля шириной w. */
static void print_centered(const char *s, int w)
{
    int len = (int)strlen(s);
    int left = (w - len) / 2;
    int right = w - len - left;
    printf("%*s%s%*s", left, "", s, right, "");
}

static void print_separator(const Board *b, int row_w, int cell_w)
{
    printf("%*s +", row_w, "");
    for (int c = 0; c < b->cols; c++) {
        for (int i = 0; i < cell_w; i++)
            putchar('-');
        putchar('+');
    }
    putchar('\n');
}

static void board_print(const Board *b)
{
    int row_w = digits(b->rows);
    int cell_w = digits(b->cols) + 2;
    char tmp[16];

    putchar('\n');

    /* заголовок с номерами столбцов */
    printf("%*s  ", row_w, "");
    for (int c = 0; c < b->cols; c++) {
        snprintf(tmp, sizeof tmp, "%d", c + 1);
        print_centered(tmp, cell_w);
        putchar(' ');
    }
    putchar('\n');

    print_separator(b, row_w, cell_w);
    for (int r = 0; r < b->rows; r++) {
        printf("%*d |", row_w, r + 1);
        for (int c = 0; c < b->cols; c++) {
            tmp[0] = board_get(b, r, c);
            tmp[1] = '\0';
            print_centered(tmp, cell_w);
            putchar('|');
        }
        putchar('\n');
        print_separator(b, row_w, cell_w);
    }
    putchar('\n');
}

/* ---------- правила ---------- */

/* Считает подряд идущие символы sym от (r,c) в направлении (dr,dc), не включая саму клетку. */
static int count_dir(const Board *b, int r, int c, int dr, int dc, char sym)
{
    int n = 0;
    r += dr;
    c += dc;
    while (r >= 0 && r < b->rows && c >= 0 && c < b->cols && board_get(b, r, c) == sym) {
        n++;
        r += dr;
        c += dc;
    }
    return n;
}

/* Проверяет, привёл ли последний ход в (r,c) к победе: горизонталь, вертикаль, обе диагонали. */
static int check_win(const Board *b, int r, int c)
{
    static const int dirs[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
    char sym = board_get(b, r, c);

    for (int i = 0; i < 4; i++) {
        int total = 1
                  + count_dir(b, r, c,  dirs[i][0],  dirs[i][1], sym)
                  + count_dir(b, r, c, -dirs[i][0], -dirs[i][1], sym);
        if (total >= b->win_len)
            return 1;
    }
    return 0;
}

/* ---------- ходы ---------- */

/* Ход человека. Возвращает 0 при EOF. */
static int human_move(Board *b, char sym, int *out_r, int *out_c)
{
    char buf[BUF_LEN];

    for (;;) {
        printf("Ход «%c» — введите строку и столбец через пробел (например: 2 3): ", sym);
        fflush(stdout);
        if (!read_line(buf, sizeof buf))
            return 0;

        int r, c;
        const char *p;
        if (!parse_int(buf, &r, &p) || !parse_int(p, &c, &p) || !only_spaces(p)) {
            printf("Ошибка: нужно ввести два целых числа — строку и столбец.\n");
            continue;
        }
        if (r < 1 || r > b->rows || c < 1 || c > b->cols) {
            printf("Ошибка: строка от 1 до %d, столбец от 1 до %d.\n", b->rows, b->cols);
            continue;
        }
        if (board_get(b, r - 1, c - 1) != EMPTY) {
            printf("Ошибка: эта клетка уже занята.\n");
            continue;
        }

        board_set(b, r - 1, c - 1, sym);
        *out_r = r - 1;
        *out_c = c - 1;
        return 1;
    }
}

/* Ход бота: случайная свободная клетка. */
static void bot_move(Board *b, char sym, int *out_r, int *out_c)
{
    int free_cnt = b->rows * b->cols - b->moves;
    int pick = rand() % free_cnt;

    for (int r = 0; r < b->rows; r++) {
        for (int c = 0; c < b->cols; c++) {
            if (board_get(b, r, c) != EMPTY)
                continue;
            if (pick-- == 0) {
                board_set(b, r, c, sym);
                *out_r = r;
                *out_c = c;
                printf("Бот («%c») ходит: строка %d, столбец %d.\n", sym, r + 1, c + 1);
                return;
            }
        }
    }
}

/* ---------- партия ---------- */

static GameResult play_round(Board *b, GameMode mode)
{
    char players[2] = { 'X', 'O' };
    int turn = 0;   /* крестики всегда ходят первыми */

    for (;;) {
        char sym = players[turn];
        int r = 0, c = 0;

        board_print(b);

        if (mode == MODE_BOT && sym == 'O') {
            bot_move(b, sym, &r, &c);
        } else if (!human_move(b, sym, &r, &c)) {
            return RESULT_EOF;
        }

        if (check_win(b, r, c)) {
            board_print(b);
            if (mode == MODE_BOT)
                printf(sym == 'X' ? "Победа! Вы выиграли («X»).\n"
                                  : "Победил бот («O»).\n");
            else
                printf("Победил игрок «%c»!\n", sym);
            return RESULT_WIN;
        }
        if (board_full(b)) {
            board_print(b);
            printf("Ничья!\n");
            return RESULT_DRAW;
        }
        turn = 1 - turn;
    }
}

/* Настройка и повторы партий. Возвращает 0 при EOF или ошибке памяти. */
static int play_session(GameMode mode)
{
    int rows, cols, win_len;

    printf("\n--- Настройка поля (от %d до %d) ---\n", MIN_DIM, MAX_DIM);
    if (!ask_int("Количество строк: ", MIN_DIM, MAX_DIM, &rows)) return 0;
    if (!ask_int("Количество столбцов: ", MIN_DIM, MAX_DIM, &cols)) return 0;

    int max_win = rows < cols ? rows : cols;
    char prompt[96];
    snprintf(prompt, sizeof prompt, "Сколько в ряд для победы (от 3 до %d): ", max_win);
    if (!ask_int(prompt, 3, max_win, &win_len)) return 0;

    for (;;) {
        Board b;
        if (!board_init(&b, rows, cols, win_len)) {
            fprintf(stderr, "Ошибка: не удалось выделить память под поле.\n");
            return 0;
        }

        printf("\nНачинаем! Поле %d x %d, для победы нужно %d в ряд. Крестики ходят первыми.\n",
               rows, cols, win_len);
        if (mode == MODE_BOT)
            printf("Вы играете за «X», бот — за «O».\n");

        GameResult res = play_round(&b, mode);
        board_free(&b);

        if (res == RESULT_EOF)
            return 0;

        int again = ask_yes_no("\nСыграть ещё раз с теми же настройками? (д/н): ");
        if (again < 0)
            return 0;
        if (!again)
            return 1;
    }
}

/* ---------- меню ---------- */

static void print_menu(void)
{
    printf("\n===== КРЕСТИКИ-НОЛИКИ =====\n");
    printf("1. Играть против бота\n");
    printf("2. Играть против другого человека\n");
    printf("3. Выйти\n");
}

int main(void)
{
    srand((unsigned)time(NULL));

    for (;;) {
        int choice;

        print_menu();
        if (!ask_int("Ваш выбор: ", 1, 3, &choice))
            break;                       /* EOF */
        if (choice == 3)
            break;

        if (!play_session(choice == 1 ? MODE_BOT : MODE_HUMAN))
            break;
    }

    printf("До свидания!");
    return 0;
}
