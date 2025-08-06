#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cmath>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <limits>
#include <SDL2/SDL.h>

#define N 100
#define MS_PER_FRAME 30
#define SCREEN_WIDTH  800
#define PADDING 1
#define SCREEN_HEIGHT 500

static bool keyboard[SDL_NUM_SCANCODES] = {0};
typedef int i32;
typedef int b32;
typedef float f32;
typedef unsigned int u32;
typedef ssize_t isize;
#define sizeof(x) ( (isize) sizeof(x) )

static u32 random_state;

static void PrintArray(std::vector<i32> a)
{
    putchar('{');
    for (isize i = 0; i < a.size(); ++i) {
        printf("%2d", a[i]);
        if (i < a.size() - 1) printf(", ");
    }
    printf("}\n");
}

static i32 SeedRandom(void) 
{
    i32 fd, ret = 0;
    if ((fd = open("/dev/urandom", 0)) >= 0) {
        if (sizeof(u32) != read(fd, &random_state, sizeof(u32))) random_state = 42;
        else ret = 1;
        close(fd);
    } else random_state = 42;
    return ret;
}

static u32 RandomUIntRanged(u32 l, u32 r)
{
    assert(l <= r);
    u32 x = random_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    random_state = x;
    return l + 1 + (x % (r - l));
}

static void ShuffleArray(std::vector<i32> &a, isize n)
{
    i32 t;
    for (i32 i = 0; i < n - 1; ++i) {
        u32 j = RandomUIntRanged(i, n - 1);
        t = a[i];
        a[i] = a[j];
        a[j] = t;
    }
}

template <class T>
class SortingAlgorithm {
public:
    virtual void Next() = 0;
    virtual bool Done() = 0;
    virtual std::vector<T> Array() const = 0;
    virtual void Sort() {
        while (!Done()) Next();
    }
};

template <class T>
class MergeSort : public SortingAlgorithm<T> {
public:
    MergeSort(std::vector<T> a) 
        : m_Arr(a), m_Buf(a.size(), 0), m_Width(1), m_Left(0), m_N(a.size()), m_Merging(0) {}

    void Next() {
restart:
        if (m_Left >= m_Arr.size() - 1) {
            m_Left = 0;
            m_Width *= 2;
        }

        if (m_Width >= m_Arr.size()) return; 
        i32 mid = std::min(m_Left + m_Width, m_N-1);
        i32 right = std::min(m_Left + 2 * m_Width, m_N);
        if (!m_Merging) {
            for (i32 i = m_Left; i < right; ++i) m_Buf[i] = m_Arr[i];
            m_I1 = m_Left, m_I2 = mid, m_K = m_Left;
            m_Merging = true;
        }

        if (m_Merging) {
            if (m_I1 < mid && m_I2 < right) {
                m_Arr[m_K++] = m_Buf[m_I1] > m_Buf[m_I2] ? m_Buf[m_I2++] : m_Buf[m_I1++];
            } else if (m_I1 < mid) {
                m_Arr[m_K++] = m_Buf[m_I1++];
            } else if (m_I2 < right) {
                m_Arr[m_K++] = m_Buf[m_I2++];
            } else {
                m_Merging = false;
                m_Left += m_Width * 2;
                goto restart;
            }
        }
    }

    bool Done() { return m_Width >= m_Arr.size(); }

    std::vector<T> Array() const { return m_Arr; }
private:
    std::vector<T> m_Arr;
    std::vector<T> m_Buf;
    i32 m_Width, m_Left, m_N, m_I1, m_I2, m_K, m_Merging;
};

i32 main() {
    SeedRandom();
    std::vector<i32> array(N);
    for (i32 i = 0; i < N; ++i) array[i] = i+1;

    ShuffleArray(array, N);
    SortingAlgorithm<i32> *m = new MergeSort{array};

    if (SDL_Init(SDL_INIT_VIDEO) < 0) return EXIT_FAILURE;
    SDL_Window *window = SDL_CreateWindow("sortviz", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        SDL_Quit();
        return EXIT_FAILURE;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        SDL_DestroyWindow(window);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_Event ev;
    b32 quit = 0;
    while (!quit) {
        u32 frame_start = SDL_GetTicks();
        f32 current_time = frame_start / 1000.0f;

        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
                case SDL_QUIT:    quit = 1;                             break;
                case SDL_KEYDOWN: keyboard[ev.key.keysym.scancode] = 1; break;
                case SDL_KEYUP:   keyboard[ev.key.keysym.scancode] = 0; break;
                default:          /* NO-OP */                           break;
            }
        }

        if (keyboard[SDL_SCANCODE_Q]) quit = 1;
        if (keyboard[SDL_SCANCODE_S]) {
            ShuffleArray(array, N);
            delete m;
            m = new MergeSort{array};
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
        std::vector<i32> arr = m->Array();
        for (i32 i = 0; i < arr.size(); ++i) {
            i32 height = ((f32)arr[i] / N) * (SCREEN_HEIGHT);
            i32 width = (f32)SCREEN_WIDTH / N;
            SDL_Rect rect = {
                (PADDING / 2) + i * width,
                SCREEN_HEIGHT - height,
                width - PADDING,
                height
            };
            SDL_RenderFillRect(renderer, &rect);
        }

        if (!m->Done()) m->Next();
        SDL_RenderPresent(renderer);
        u32 elapsed = SDL_GetTicks() - frame_start;
        if (elapsed < MS_PER_FRAME) SDL_Delay(MS_PER_FRAME - elapsed);
    }
    delete m;
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
