#include <iostream>
#include <chrono>
#include <windows.h>
#include <chrono>
#include <string>
#include <thread> 
#include <mutex>
#include <condition_variable>
#include <windows.h>
#include <winnt.h>
#include "SerialPort.h"
#include "WinDesktopDup.h"


WinDesktopDup dup;

int screenW = 0; // width of main monitor
int screenH = 0; // height of main monitor
HDC hScreen = NULL;
HDC hdcMem = NULL;
HBITMAP hBitmap = NULL;
HGDIOBJ hOld = NULL;
BITMAPINFOHEADER bmi = { 0 };
uint8_t *screen; // monitor pixels BGRA matrix (RGBA inverted)

const uint8_t HLEDS = 30; // # of leds on upper/lower side of screen
const uint8_t VLEDS = 15; // # of leds on right/left side of screen
const int LEDTOT = (HLEDS * 2) + (VLEDS * 2); // monitor has 4 sides
char ledValues[(LEDTOT * 3 * 3) + 1] = { '0' }; // RGB 8bpc values in physical order + \n
std::string values(LEDTOT * 3, '0');

//[(LEDTOT * nOfChannels * charsPerChannel)], no commas
// example: [25525525517052001255255255] --> led n.2 is [170,52,1]

bool terminates = false;
int t_count = 0; // thread counter
std::mutex mu_t_count; // protects t_count
std::mutex mu_exec;
std::condition_variable cv_count;
std::condition_variable cv_execute;

inline int PosB(int x, int y)
{
    return screen[4 * ((y * screenW) + x)];
}

inline int PosG(int x, int y)
{
    return screen[4 * ((y * screenW) + x) + 1];
}

inline int PosR(int x, int y)
{
    return screen[4 * ((y * screenW) + x) + 2];
}

void ScreenCap()
{
    hScreen = GetDC(NULL);
    hdcMem = CreateCompatibleDC(hScreen);
    hBitmap = CreateCompatibleBitmap(hScreen, screenW, screenH);
    hOld = SelectObject(hdcMem, hBitmap);
    BitBlt(hdcMem, 0, 0, screenW, screenH, hScreen, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOld);

    GetDIBits(hdcMem, hBitmap, 0, screenH, screen, (BITMAPINFO*)&bmi, DIB_RGB_COLORS); // the slow part

    ReleaseDC(GetDesktopWindow(), hScreen);
    DeleteDC(hdcMem);
    DeleteObject(hBitmap);
}

bool ButtonPress(int Key)
{
    bool button_pressed = false;

    while (GetAsyncKeyState(Key))
        button_pressed = true;

    return button_pressed;
}

void init()
{
    hScreen = GetDC(NULL);
    screenW = GetDeviceCaps(hScreen, HORZRES);
    screenH = GetDeviceCaps(hScreen, VERTRES);

    printf("Display is %dx%d.\n", screenW, screenH);

    bmi.biSize = sizeof(BITMAPINFOHEADER);
    bmi.biPlanes = 1;
    bmi.biBitCount = 8;
    bmi.biWidth = screenW;
    bmi.biHeight = -screenH;

    screen = (BYTE*) malloc(screenW * screenH * 4); // TODO free here at the end? uint8_t instead of BYTE?
    if (!screen)
        printf("Can't allocate requested buffer");

    printf("Allocated %d bytes as screnshot memory\n", screenW * screenH * 4 * (bmi.biBitCount / 8));
}

void computeTileAvgColor(int startX, int startY, int tileWidth, int tileHeight, int step, int ledIndex)
{
    int yCoord = 0;
    int totPixel = (tileWidth / step) * (tileHeight / step) ;
    int r = 0, g = 0, b = 0;

    std::string red;
    std::string green;
    std::string blue;

    for (int y = startY; y < startY + tileHeight; y += step)
    {
        yCoord = y * screenW;
        for (int x = startX; x < startX + tileWidth; x += step)
        {
            r += screen[4 * (yCoord + x) + 2];
            g += screen[4 * (yCoord + x) + 1];
            b += screen[4 * (yCoord + x)];
        }
    }

    r /= totPixel;
    g /= totPixel;
    b /= totPixel;

    red = std::to_string(r);
    green = std::to_string(g);
    blue = std::to_string(b);

    // add leading zeroes padding if needed
    if (r < 10)
        red.insert(0, 2, '0');
    else if (r < 100)
        red.insert(0, 1, '0');

    if (r < 10)
        green.insert(0, 2, '0');
    else if (r < 100)
        green.insert(0, 1, '0');

    if (r < 10)
        blue.insert(0, 2, '0');
    else if (r < 100)
        blue.insert(0, 1, '0');

    // save result to be sent over serial port
    ledIndex *= 3;
    ledValues[ledIndex + 0] = red[0];
    ledValues[ledIndex + 1] = red[1];
    ledValues[ledIndex + 2] = red[2];
    ledValues[ledIndex + 3] = green[0];
    ledValues[ledIndex + 4] = green[1];
    ledValues[ledIndex + 5] = green[2];
    ledValues[ledIndex + 6] = blue[0];
    ledValues[ledIndex + 7] = blue[1];
    ledValues[ledIndex + 8] = blue[2];
}

void drawRectangle(int r, int g, int b, int startX, int startY, int endX, int endY)
{
    HDC screenDC = GetDC(NULL); //NULL gets whole screen
    HBRUSH brush = CreateSolidBrush(RGB(r, g, b)); //create brush
    SelectObject(screenDC, brush); //select brush into DC
    Rectangle(screenDC, startX, startY, endX, endY); //draw rectangle over whole screen

    //clean up stuff here
    DeleteDC(screenDC);
}

void drawPixel(int r, int g, int b, int x, int y)
{
    HDC screenDC = GetDC(NULL); //NULL gets whole screen
    //SelectObject(screenDC); //select brush into DC
    SetPixel(screenDC, x, y, RGB(r, g, b));

    //clean up stuff here
    DeleteDC(screenDC);
}

void threadWork(int startX, int startY, int tileWidth, int tileHeight, int step, int ledIndex)
{
    while (!terminates)
    {
        // start sync barrier
        std::unique_lock <std::mutex> locker1(mu_exec); // ask to access var
        cv_execute.wait(locker1); // auto releases the locker1 when waking

        // work ...
        computeTileAvgColor(startX, startY, tileWidth, tileHeight, step, ledIndex);
        //printf("executed %d \n", ledIndex);

        // end sync barrier       
        std::unique_lock <std::mutex> locker2(mu_t_count); // ask to access var
        t_count--;
        if(t_count == 0) cv_count.notify_one(); // sad this can't be optimized
        locker2.unlock();
    }
}

int copyScreen()
{
    // copies the top left corner of the first screen into the second screen
    // assuming a two-monitor setup where left monitor is primary and right one is secondary

    init();
    bool res = dup.Initialize();

    if (!res)
        return -1;


    int r = 0, g = 0, b = 0;
    int yCoord = 0;

    HDC screenDC = GetDC(NULL); //NULL gets whole screen, neede for SetPixel(), not part of Desktop Duplication API

    while (true)
    {
        dup.CaptureFrame(screen);

        //printf("Top left pixel is: [%d,%d,%d]\n", screen[2], screen[1], screen[0]);

        for (int y = 0; y < 200; y += 1)
        {
            yCoord = y * screenW;
            for (int x = 0; x < 200; x += 1)
            {
                r = screen[4 * (yCoord + x) + 2];
                g = screen[4 * (yCoord + x) + 1];
                b = screen[4 * (yCoord + x)];
                SetPixel(screenDC, x + screenW, y, RGB(r, g, b)); // slow part
            }
        }
    }

    DeleteDC(screenDC);

    return 0;
 }

int main()
{
    init();

    char comport[] = "COM1";

    SerialPort serialPort(&comport[0], 115200);
    if(!serialPort.isConnected())
        return -1;

    if(!dup.Initialize())
        return -1;

    std::thread threads[LEDTOT];

    // create upper side threads
    for (int t = 0; t < LEDTOT; t++)
        threads[t] = std::thread(threadWork, 1 * t, 0, 150, 150, 1, t);


    ledValues[(LEDTOT * 3 * 3)] = '\n';
            
    while (true)
    {
        dup.CaptureFrame(screen);              
        
        // wake up threads
        t_count = LEDTOT;
        cv_execute.notify_all();

        // wait for threads to finish
        std::unique_lock <std::mutex> locker(mu_t_count); // ask to access var
        cv_count.wait(locker, []() {return t_count == 0;}); // auto releases the locker1 when waking

        //std::cout << ledValues << std::endl;
        // transmit over UART
        auto t1 = std::chrono::high_resolution_clock::now();
        serialPort.Write(ledValues, strlen(ledValues) + 1);
        //printf("------\n");

        auto t2 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << ms_double.count()/1000.00 << "ms\n";
    }
        
    for (uint8_t t = 0; t < LEDTOT; t++)
        threads[t].join();

    free(screen);

    return 0;
}

int mainn()
{
    int frames = 0;
    int iterations = 160;

    init();

    while(true)
    {        
        //ScreenCap();
        //computeTileAvgColor(0, 500, 150, 150, 1, 0);
        //drawRectangle(255,0,0, 150,500,153,500+150);
        //drawRectangle(ledValues[0], ledValues[1], ledValues[2], 1920, 500, 1920+150, 500+150);
        //printf("Update with [%d,%d,%d] \n", ledValues[0], ledValues[1], ledValues[2]);
      
        auto t1 = std::chrono::high_resolution_clock::now();
        ScreenCap();
        for (int a = 0; a < 160; a++)
        {            
            computeTileAvgColor(150*a, 500, 150, 150, 5, a);          
        }
        auto t2 = std::chrono::high_resolution_clock::now();

        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << ms_double.count()/1000.00 << "ms\n";
    }
    

/*    while (true)
    {
        if (ButtonPress(VK_SPACE))
        {
            POINT p;
            GetCursorPos(&p);
            ScreenCap();
            computeTileAvgColor(0, 0, 300, 300, 90000, 0);

            printf("%d,%d,%d\n", ledValues[0], ledValues[1], ledValues[2]);
            //std::cout << "Bitmap: r: " << PosR(p.x, p.y) << " g: " << PosG(p.x, p.y) << " b: " << PosB(p.x, p.y) << "\n";

        }
        else if (ButtonPress(VK_ESCAPE))
        {
            if (ScreenData)
                free(ScreenData);

            printf("Quit\n");
            break;
        }
    }*/

    return 0;
}
