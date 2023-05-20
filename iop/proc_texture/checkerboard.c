#include "checkerboard.h"

/*
static int height = 128;

static int width = 128;

static int channels = 3; */

static int color[3] = {255, 0, 0};



int CheckerBoard(int x, int y, int context)
{
    int tempX = 5 * x;
    int modX = tempX - my_floor(tempX);

    int ret1 = 0;
    int ret2 = 255;


    int tempY = 5 * y;
    int modY = tempY - my_floor(tempY);

   int samp_x = (int)(0.5 * FIXED_P_ONE);
     int samp_y = (int)(0.5 * FIXED_P_ONE);

    if (context == 1)
    {
        ret1 = 1;
        ret2 = 0;
    } else {
        ret1 = 0;
        ret2 = 1;
    }

    if ((modX < samp_x) ^ (modY < samp_y))
    {
        return ret1;
    } else {
        return ret2;
    }
}


void generateBoard(unsigned char *iop_addr, int buffer_size, int height, int channels, int context, int angle)
{
    unsigned int resbits;

    int index;

    int width = height;

    int over_x_fp = (FIXED_P_ONE / width);

    int over_y_fp =  (FIXED_P_ONE / height);

    int angle_x, angle_y;




        angle_x = icos(angle);
        angle_y = isin(angle);






        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {

                int x1 = (over_x_fp * j);
                int y1 = (over_y_fp * i);



               int s = ((x1 * angle_x) - (y1 * angle_y))>>12;
                int t = ((y1 * angle_x) + (x1 * angle_y))>>12;


                int res = CheckerBoard(s, t, context);

                for (int k = 0; k < channels; k++)
                {
                    index = ((i * (width * channels)) + (channels * j)) + k;




                   //float res_f = res / FIXED_P_ONE;

                    int finalColor = color[k] * res;
                    //printf("%d\n", res);
                   iop_addr[index] = finalColor;

                }
            }
        }


}
