#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <raylib.h>

typedef struct vec2i
{
    int x;
    int y;
} vec2i;

typedef struct
{
    int arr_size;
    int offset_from_start;
    int fw; // Width of image
    int fh; // Height of image

} PPM_File_Data;

uint8_t *read_PPM(const char *file_name, PPM_File_Data *data)
{
    FILE *test_file = fopen(file_name, "rb");

    if (test_file == NULL)
    {
        perror("Error opening file");
        return NULL;
    }

    int type_read = 0;
    int P6 = 0x50 + 0x36; // hex('P') + hex('6')

    int file_width = -1;
    int file_height = -1;
    int max_col = -1;

    fseek(test_file, 0, SEEK_END);
    int file_size = ftell(test_file);

    rewind(test_file);

    uint8_t *image_data = (uint8_t *)calloc(file_size, sizeof(uint8_t));

    int i = 0;

    uint8_t newline = 0x0A; // For easy access
    uint8_t space = 0x20;

    uint8_t hashtag = 0x23;
    bool comment = false;

    int buffer = 0;

    bool img_valid = false;

    uint8_t eval_stack[10]; // For converting ASCII numbers to decimal
    int stack_top = -1;

    // Read the file and check for headers
    while (fread(&image_data[i], 1, 1, test_file))
    {
        if (i == 0 || i == 1)
        {
            type_read += image_data[i];

            if (type_read != P6 && i == 1)
            {
                printf("File is not a ppm image.\n");
                free(image_data);
                return NULL;
            }
        }

        if (image_data[i] == hashtag)
        {
            comment = true;
        }

        if (comment == false && i > 1 && type_read == P6)
        {
            if (image_data[i] == newline || image_data[i] == space)
            {
                if (stack_top == -1)
                {
                    i++;
                    continue;
                }

                // Unwind stack
                int mult = 1;

                while (stack_top != -1)
                {
                    int calc = eval_stack[stack_top] - 0x30; // 0x30 is decimal '0' in ASCII

                    buffer += mult * calc;

                    mult *= 10;

                    stack_top--;
                }

                if (file_width < 0)
                {
                    file_width = buffer;
                }
                else if (file_height < 0)
                {
                    file_height = buffer;
                }
                else if (max_col < 0)
                {
                    max_col = buffer;
                }

                buffer = 0;
            }

            else
            {
                stack_top++;
                eval_stack[stack_top] = image_data[i];
            }
        }

        else
        {
            if (image_data[i] == newline)
            {
                comment = false;
            }
        }

        if (type_read == P6 && file_width > 0 && file_height > 0 && max_col > 0)
        {
            img_valid = true;
            break;
        }

        i++;
    }

    if (img_valid)
    {
        printf("File Type: %d\nResolution: %dx%d\nColor: %d\n", type_read, file_width, file_height, max_col);

        data->fw = file_width;
        data->fh = file_height;
    }
    else
    {
        printf("Invalid Format\n");
        free(image_data);
        return NULL;
    }

    data->offset_from_start = i; // For freeing;

    uint8_t *pixels = image_data + i;

    i = 0;

    while (fread(&pixels[i], 1, 1, test_file))
    {
        i++;
    }

    data->arr_size = i; // Will be used for calculating when to loop back around the pixels array

    fclose(test_file);

    return pixels;
}

int main()
{
    PPM_File_Data data; // Packages useful information about the PPM
    uint8_t *pixels = read_PPM("testfiles/test.ppm", &data);

    if (pixels != NULL)
    {

        const int screen_width = 1920;
        const int screen_height = 1080;

        InitWindow(screen_width, screen_height, "PPM Image viewer");

        vec2i cursor_posn = {(screen_width - data.fw) / 2, (screen_height - data.fh) / 2}; // Sets the top-left corner for drawing dynamically

        while (!WindowShouldClose())
        {
            BeginDrawing();

            ClearBackground(BLACK);

            int cursor_index = 0;
            vec2i cur_cursor = {cursor_posn.x, cursor_posn.y};

            while (cursor_index < data.arr_size)
            {
                DrawPixel(cur_cursor.x, cur_cursor.y, (Color){pixels[cursor_index], pixels[cursor_index + 1], pixels[cursor_index + 2], 0xFF});

                cursor_index += 3;

                cur_cursor.x = cur_cursor.x + 1;
                if (cursor_index % (data.fw * 3) == 0)
                {
                    cur_cursor.x = cursor_posn.x;
                    cur_cursor.y = cur_cursor.y + 1;
                }
            }

            EndDrawing();
        }

        CloseWindow();
    }

    free(pixels - data.offset_from_start);

    return 0;
}