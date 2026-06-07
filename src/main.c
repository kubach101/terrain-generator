#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <stdlib.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <float.h>
#define ANIM_SIZE 800
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
typedef struct
{
    float x, y, z;
} vec3;

typedef struct
{
    vec3 v[3];
} triangle;

typedef struct
{
    float x, y;
} vec2;

typedef struct
{
    vec2 v[3];
    float d;
    SDL_FColor color;
} triangle_flat;

typedef struct
{
    float freq;
    float amp;
    float px;
    float pz;
} noise_layer;

typedef struct
{
    float x;
    float z;
    float h;
    float sharp;
} mountain;

typedef struct
{
    float min, max;
} range;

typedef struct
{
    int min, max;
} int_range;

typedef struct
{
    int_range num;
    range h, sharp;
    float reverse_prob;
} mtype_parameters;

typedef struct
{
    SDL_FColor color[7];
} color_pallete;

vec2 flat_projection(vec3 *v, float camera_z)
{
    float z = v->z - camera_z;
    vec2 fv = {v->x / z, v->y / z};
    return fv;
}
void convert_to_px(vec2 *v)
{
    v->x = (v->x + 1) * ANIM_SIZE / 2 + (SCREEN_WIDTH - ANIM_SIZE) / 2;
    v->y = (1 - v->y) * ANIM_SIZE / 2 + (SCREEN_HEIGHT - ANIM_SIZE) / 2;
}
void render_triangle(triangle_flat *t, SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, t->color.r * 255, t->color.g * 255, t->color.b * 255, t->color.a * 255);
    SDL_RenderLine(renderer, t->v[0].x, t->v[0].y, t->v[1].x, t->v[1].y);
    SDL_RenderLine(renderer, t->v[0].x, t->v[0].y, t->v[2].x, t->v[2].y);
    SDL_RenderLine(renderer, t->v[1].x, t->v[1].y, t->v[2].x, t->v[2].y);
}

void setup(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
}
void rotate_y(vec3 *v, float angle)
{
    float c = cos(angle);
    float s = sin(angle);

    float x = v->x;
    float z = v->z;

    v->x = x * c - z * s;
    v->z = x * s + z * c;
}
void rotate_x(vec3 *v, float angle)
{
    float c = cos(angle);
    float s = sin(angle);

    float y = v->y;
    float z = v->z;

    v->y = y * c - z * s;
    v->z = y * s + z * c;
}
void rotate_z(vec3 *v, float angle)
{
    float c = cos(angle);
    float s = sin(angle);

    float x = v->x;
    float y = v->y;

    v->x = x * c - y * s;
    v->y = x * s + y * c;
}
float randf(float min, float max)
{
    return min +
           ((float)rand() / RAND_MAX) * (max - min);
}
triangle *mesh_gen(int mesh_size, float mesh_step)
{
    float x0 = -mesh_step * mesh_size / 2;
    float z = x0;
    int ti = 0;
    triangle *mesh =
        malloc(sizeof(triangle) * mesh_size * mesh_size * 2);

    for (int rows = 0; rows < mesh_size; rows++)
    {
        float x = x0;
        float zs = z + mesh_step;

        for (int cols = 0; cols < mesh_size; cols++)
        {
            float xs = x + mesh_step;

            triangle t1 = {{{x, 0, z},
                            {x, 0, zs},
                            {xs, 0, z}}};

            triangle t2 = {{{xs, 0, zs},
                            {xs, 0, z},
                            {x, 0, zs}}};

            mesh[ti++] = t1;
            mesh[ti++] = t2;

            x += mesh_step;
        }

        z += mesh_step;
    }

    return mesh;
}
float determine_height_from_noise(float x, float z, noise_layer *layers, int l_num)
{
    float y = 0.0f;

    for (int li = 0; li < l_num; li++)
    {
        y +=
            sinf(x * layers[li].freq + layers[li].px) * cosf(z * layers[li].freq + layers[li].pz) * layers[li].amp;
    }

    return y;
}
void noise_layers_gen(noise_layer *layers, int l_num)
{
    for (int li = 0; li < l_num; li++)
    {
        layers[li].freq =
            randf(0.5f, 2.5f) * powf(2.0f, li) * randf(0.15f, 1.3f);

        layers[li].amp =
            randf(0.01f, 0.2f) * powf(0.5, li) * randf(0.3f, 0.45f);

        layers[li].px =
            randf(0.0f, 1000.0f);

        layers[li].pz =
            randf(0.0f, 1000.0f);
    }
}
float determine_height_from_mountain(
    float x,
    float z,
    mountain *m)
{
    float dx = x - m->x;
    float dz = z - m->z;

    float dist2 = dx * dx + dz * dz;

    return m->h * expf(-dist2 * m->sharp);
}

mountain mountain_rand(mtype_parameters *p, float mesh_length)
{
    mountain t;
    t.x = randf(-mesh_length / 2 + (mesh_length / 8), mesh_length / 2 - (mesh_length / 8));
    t.z = randf(-mesh_length / 2 + (mesh_length / 8), mesh_length / 2 - (mesh_length / 8));
    t.h = randf(p->h.min, p->h.max);
    t.sharp = randf(p->sharp.min, p->sharp.max);
    // if (randf(0.0f, 1.0f) <= p->reverse_prob) t.h *= -1.0f;
    return t;
}
void mountains_all(mountain *mountains, mtype_parameters *big, mtype_parameters *med, mtype_parameters *sml, float mesh_lengh, int *m_num)
{
    int mi = 0;
    int num_big = rand() % (big->num.max - big->num.min + 1) + big->num.min;
    int num_med = rand() % (med->num.max - med->num.min + 1) + med->num.min;
    int num_sml = rand() % (sml->num.max - sml->num.min + 1) + sml->num.min;
    *m_num = num_big + num_med + num_sml;
    for (int i = 0; i < num_big; i++)
    {
        mountains[mi++] = mountain_rand(big, mesh_lengh);
    }

    for (int i = 0; i < num_med; i++)
    {
        mountains[mi++] = mountain_rand(med, mesh_lengh);
    }

    for (int i = 0; i < num_sml; i++)
    {
        mountains[mi++] = mountain_rand(sml, mesh_lengh);
    }
}
void mesh_shape(triangle *mesh, int t_num, mountain *mountains, int m_num, noise_layer *layers, int l_num)
{
    for (int ti = 0; ti < t_num; ti++)
    {
        triangle t = mesh[ti];
        for (int vi = 0; vi < 3; vi++)
        {
            t.v[vi].y += determine_height_from_noise(t.v[vi].x, t.v[vi].z, layers, l_num);
            for (int mi = 0; mi < m_num; mi++)
            {
                t.v[vi].y += determine_height_from_mountain(t.v[vi].x, t.v[vi].z, &mountains[mi]);
            }
        }
        mesh[ti] = t;
    }
}
SDL_FColor determine_color_from_height(float y, range hr, color_pallete *pallete)
{
    SDL_FColor c;

    float t = (y - hr.min) / (hr.max - hr.min);
    if (t < 0.0f)
        t = 0.0f;
    if (t > 1.0f)
        t = 1.0f;

    int n = 6;
    float scaled = t * n;
    int i = (int)scaled;
    float f = scaled - i;

    if (i >= n)
    {
        i = n - 1;
        f = 1.0f;
    }

    SDL_FColor a_col = pallete->color[i];
    SDL_FColor b_col = pallete->color[i + 1];

    c.r = a_col.r + f * (b_col.r - a_col.r);
    c.g = a_col.g + f * (b_col.g - a_col.g);
    c.b = a_col.b + f * (b_col.b - a_col.b);
    c.a = 1.0f;
    return c;
}
float bigest_height_value(triangle *t)
{
    float mh = 0.0f;
    if (t->v[0].y + t->v[1].y + t->v[2].y < 0)
    {
        for (int vi = 0; vi < 3; vi++)
        {
            mh = (t->v[vi].y < mh) ? t->v[vi].y : mh;
        }
    }
    else
    {
        for (int vi = 0; vi < 3; vi++)
        {
            mh = (t->v[vi].y > mh) ? t->v[vi].y : mh;
        }
    }
    return mh;
}

float avg_depth(triangle *t)
{
    return (t->v[0].z + t->v[1].z + t->v[2].z) / 3.0f;
}
int compare_triangle_depth(const void *a, const void *b)
{
    triangle_flat *ta = (triangle_flat *)a;
    triangle_flat *tb = (triangle_flat *)b;
    if (tb->d > ta->d)
        return 1;
    if (tb->d < ta->d)
        return -1;
    return 0;
}
void height_range_update(range *hr, float y)
{
    hr->max = (hr->max < y) ? y : hr->max;
    hr->min = (hr->min > y) ? y : hr->min;
}

void get_terrain_height_range(range *hr, triangle *terrain, int t_num)
{
    hr->max = -FLT_MAX;
    hr->min = FLT_MAX;
    for (int ti = 0; ti < t_num; ti++)
    {
        for (int vi = 0; vi < 3; vi++)
        {
            height_range_update(hr, terrain[ti].v[vi].y);
        }
    }
}

void convert_to_vertex_buffer(SDL_Vertex *v_buffer, triangle_flat *ft_buffer, int passed_t_num)
{
    for (int ti = 0; ti < passed_t_num; ti++)
    {
        triangle_flat ft = ft_buffer[ti];
        for (int vi = 0; vi < 3; vi++)
        {
            int idx = ti * 3 + vi;
            v_buffer[idx].position.x = ft.v[vi].x;
            v_buffer[idx].position.y = ft.v[vi].y;
            v_buffer[idx].color.r = ft.color.r;
            v_buffer[idx].color.g = ft.color.g;
            v_buffer[idx].color.b = ft.color.b;
            v_buffer[idx].color.a = ft.color.a;
        }
    }
}

int main(int argc, char *argv[])
{
    unsigned world_seed = 0;
    srand(world_seed);
    float dt = 0.0f;
    // camera
    float angle_y = -M_PI / 4;
    float angle_x = -M_PI / 4;
    float angle_z = 0.0f;
    float camera_z = -9.2f;
    float min_dist = 0.2f;

    // mesh parameters
    int mesh_size = 60; // default
    float mesh_length = 9.0f;

    // usr input
    printf("Enter the mesh size\n>");
    scanf(" %d", &mesh_size);
    if (mesh_size <= 0 || mesh_size > 160)
        mesh_size = 60;
    float mesh_step = mesh_length / mesh_size;
    int t_num = 2 * mesh_size * mesh_size;

    // mountains parameters
    mtype_parameters big = {{1, 3}, {1.2f, 2.0f}, {0.1f, 1.2f}, 0.05f};
    mtype_parameters med = {{5, 9}, {0.5f, 1.2f}, {1.2f, 2.5f}, 0.15f};
    mtype_parameters sml = {{7, 16}, {0.05f, 0.5f}, {2.5f, 5.0f}, 0.3f};
    int max_m_num = big.num.max + med.num.max + sml.num.max;
    int m_num = 0;

    // noise parameters
    int l_num = 8;

    // creating terrain
    triangle *mesh = mesh_gen(mesh_size, mesh_step);
    triangle *terrain = malloc(sizeof(triangle) * t_num);
    mountain *mountains = malloc(sizeof(mountain) * max_m_num);
    mountains_all(mountains, &big, &med, &sml, mesh_length, &m_num);
    noise_layer *layers = malloc(sizeof(noise_layer) * l_num);
    noise_layers_gen(layers, l_num);
    memcpy(terrain, mesh, sizeof(triangle) * t_num);
    mesh_shape(terrain, t_num, mountains, m_num, layers, l_num);

    // buffers
    triangle_flat *ft_buffer = malloc(sizeof(triangle_flat) * t_num);
    SDL_Vertex *v_buffer = malloc(sizeof(SDL_Vertex) * t_num * 3);

    // ranges
    range hr;
    get_terrain_height_range(&hr, terrain, t_num);

    // color palletes
    color_pallete thermal = {{
        {0.00f, 0.00f, 0.00f, 1.0f},
        {0.20f, 0.00f, 0.30f, 1.0f},
        {0.60f, 0.00f, 0.50f, 1.0f},
        {1.00f, 0.10f, 0.20f, 1.0f},
        {1.00f, 0.50f, 0.00f, 1.0f},
        {1.00f, 0.85f, 0.00f, 1.0f},
        {1.00f, 1.00f, 1.00f, 1.0f},
    }};

    color_pallete bgr = {{
        {0.00f, 0.00f, 1.00f, 1.0f},
        {0.00f, 0.50f, 1.00f, 1.0f},
        {0.00f, 1.00f, 0.50f, 1.0f},
        {0.50f, 1.00f, 0.00f, 1.0f},
        {1.00f, 1.00f, 0.00f, 1.0f},
        {1.00f, 0.50f, 0.00f, 1.0f},
        {1.00f, 0.00f, 0.00f, 1.0f},
    }};
    color_pallete rocks = {{
        {0.10f, 0.22f, 0.08f, 1.0f},
        {0.24f, 0.36f, 0.16f, 1.0f},
        {0.42f, 0.36f, 0.24f, 1.0f},
        {0.48f, 0.43f, 0.38f, 1.0f},
        {0.59f, 0.55f, 0.51f, 1.0f},
        {0.72f, 0.70f, 0.67f, 1.0f},
        {0.94f, 0.93f, 0.93f, 1.0f},
    }};

    color_pallete sand = {{
        {0.78f, 0.66f, 0.30f, 1.0f},
        {0.83f, 0.72f, 0.42f, 1.0f},
        {0.78f, 0.58f, 0.35f, 1.0f},
        {0.73f, 0.45f, 0.28f, 1.0f},
        {0.62f, 0.37f, 0.22f, 1.0f},
        {0.48f, 0.31f, 0.19f, 1.0f},
        {0.91f, 0.86f, 0.78f, 1.0f},
    }};

    color_pallete lava = {{
        {0.10f, 0.10f, 0.10f, 1.0f},
        {0.24f, 0.06f, 0.00f, 1.0f},
        {0.43f, 0.12f, 0.00f, 1.0f},
        {0.62f, 0.20f, 0.00f, 1.0f},
        {0.82f, 0.31f, 0.00f, 1.0f},
        {1.00f, 0.47f, 0.00f, 1.0f},
        {1.00f, 0.87f, 0.27f, 1.0f},
    }};
    color_pallete edgepallete_set[] = {thermal, bgr};
    color_pallete fillpallete_set[] = {rocks, sand, lava};
    int fillpallete_num = sizeof(fillpallete_set) / sizeof(color_pallete);
    int edgepallete_num = sizeof(edgepallete_set) / sizeof(color_pallete);

    // initialization
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        SDL_Log("Inicialization error: %s", SDL_GetError());
        free(mesh);
        free(terrain);
        free(ft_buffer);
        free(v_buffer);
        free(mountains);
        free(layers);
        return 1;
    }
    SDL_Window *window;
    window = SDL_CreateWindow("Terrain generator by Kuba Chmura", SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (window == NULL)
    {
        SDL_Log("Error creating a window: %s", SDL_GetError());
        free(mesh);
        free(terrain);
        free(ft_buffer);
        free(v_buffer);
        free(mountains);
        free(layers);
        return 1;
    }
    SDL_Renderer *renderer;
    renderer = SDL_CreateRenderer(window, NULL);
    if (renderer == NULL)
    {
        SDL_Log("Error creating a renderer: %s", SDL_GetError());
        free(mesh);
        free(terrain);
        free(ft_buffer);
        free(v_buffer);
        free(mountains);
        free(layers);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_RaiseWindow(window);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
    bool is_running = true;
    bool regenerate = false;
    bool auto_rotate = false;
    bool fill = true;
    color_pallete pallete;
    int fillpallete_selector = 0;
    int edgepallete_selector = 0;
    float refresh_timer = 0.0f;
    SDL_Event event;
    Uint64 last_time = SDL_GetPerformanceCounter();
    Uint64 current_time = 0;
    while (is_running)
    {
        angle_y = fmod(angle_y, 2 * M_PI);
        angle_x = fmod(angle_x, 2 * M_PI);
        angle_z = fmod(angle_z, 2 * M_PI);
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            {
                is_running = false;
            }

            if (event.type == SDL_EVENT_KEY_DOWN)
            {

                switch (event.key.key)
                {
                case SDLK_SPACE:
                    regenerate = true;
                    break;
                case SDLK_LEFT:
                    angle_y += M_PI / 180;
                    break;
                case SDLK_UP:
                    angle_x -= M_PI / 180;
                    break;
                case SDLK_RIGHT:
                    angle_y -= M_PI / 180;
                    break;
                case SDLK_DOWN:
                    angle_x += M_PI / 180;
                    break;
                case SDLK_C:
                    auto_rotate = !auto_rotate;
                    break;
                case SDLK_X:
                    angle_y = -M_PI / 4;
                    angle_x = -M_PI / 4;
                    camera_z = -9.2f;
                    break;
                case SDLK_Z:
                    fill = !fill;
                    fillpallete_selector = 0;
                    edgepallete_selector = 0;
                    break;
                case SDLK_V:
                    if (fill)
                    {
                        fillpallete_selector++;
                        fillpallete_selector %= fillpallete_num;
                    }
                    else
                    {
                        edgepallete_selector++;
                        edgepallete_selector %= edgepallete_num;
                    }
                    break;
                case SDLK_M:
                    camera_z += 0.15f;
                    if (camera_z > 0)
                        camera_z = 0;
                    break;
                case SDLK_N:
                    camera_z -= 0.15f;
                    if (camera_z < -24.0f)
                        camera_z = -24.0f;
                    break;
                default:
                    break;
                }
            }
        }
        if (regenerate)
        {
            regenerate = false;
            memcpy(terrain, mesh, sizeof(triangle) * t_num);
            world_seed = world_seed * 1664525u + 1013904223u;
            srand(world_seed);
            mountains_all(mountains, &big, &med, &sml, mesh_length, &m_num);
            noise_layers_gen(layers, l_num);
            mesh_shape(terrain, t_num, mountains, m_num, layers, l_num);
            regenerate = false;
            get_terrain_height_range(&hr, terrain, t_num);
        }
        if (auto_rotate)
        {
            angle_y += -M_PI * dt / 2;
        }
        if (fill)
            pallete = fillpallete_set[fillpallete_selector];
        else
            pallete = edgepallete_set[edgepallete_selector];
        current_time = SDL_GetPerformanceCounter();
        dt = (current_time - last_time) / (float)SDL_GetPerformanceFrequency();
        last_time = current_time;
        float fps = 1.0f / dt;
        refresh_timer += dt;
        if (refresh_timer >= 0.25f)
        {
            printf("\rFPS: %.2f", fps);
            fflush(stdout);
            refresh_timer = 0.0f;
        }
        setup(renderer);
        int passed_t_count = 0;
        for (int ti = 0; ti < t_num; ti++)
        {
            triangle t = terrain[ti];
            for (int vi = 0; vi < 3; vi++)
            {
                rotate_y(&t.v[vi], angle_y);
                rotate_x(&t.v[vi], angle_x);
                rotate_z(&t.v[vi], angle_z);
                if (t.v[vi].z < camera_z + min_dist)
                    goto skip_triangle;
            }
            triangle_flat ft;
            for (int vi = 0; vi < 3; vi++)
            {
                ft.v[vi] = flat_projection(&t.v[vi], camera_z);
                convert_to_px(&ft.v[vi]);
            }
            ft.d = avg_depth(&t);
            ft.color = determine_color_from_height(bigest_height_value(&terrain[ti]), hr, &pallete);
            ft_buffer[passed_t_count++] = ft;
        skip_triangle:;
        }
        qsort(ft_buffer, passed_t_count, sizeof(triangle_flat), compare_triangle_depth);
        if (!fill)
        {
            for (int ti = 0; ti < passed_t_count; ti++)
            {
                render_triangle((ft_buffer + ti), renderer);
            }
        }
        if (fill)
        {
            convert_to_vertex_buffer(v_buffer, ft_buffer, passed_t_count);
            SDL_RenderGeometry(renderer, NULL, v_buffer, passed_t_count * 3, NULL, passed_t_count * 3);
        }
        SDL_RenderPresent(renderer);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    free(mesh);
    free(terrain);
    free(ft_buffer);
    free(v_buffer);
    free(mountains);
    free(layers);
    return 0;
}
