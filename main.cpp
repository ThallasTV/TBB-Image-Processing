#include <iostream>
#include <vector>
//Thread building blocks library
#include <tbb/task_scheduler_init.h>
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_reduce.h>
#include <tbb/tick_count.h>
//Free Image library
#include <FreeImagePlus.h>
#include <random>

using namespace std;
using namespace tbb;

//function which contains the calculatoin for Gaussian blur
float Gaussian2D(int x, int y, float sigma) {
    return 1.0f / (2.0f * float(M_PI) * sigma * sigma) * exp(-((x * x + y * y) / (2.0f * sigma * sigma)));
}


void part1GaussianBlur()
{
    fipImage inputImage; //creating an inputImage
    inputImage.load("../Images/Salisbury_lowres.jpg"); //loading the image from the Image directory
    inputImage.convertToFloat(); // Converting to float to perform pixel blurring

    const int width = inputImage.getWidth(); // width = width of image
    const int height = inputImage.getHeight(); // height = height of the image

    uint64_t numElements = width * height; //gets the total array size

    fipImage outputImage; // creates and output image
    outputImage = fipImage(FIT_FLOAT, width, height, 32);
    float *outputBuffer = (float*)outputImage.accessPixels(); // Allows the pixels of the output image to be accessible
    float *inputBuffer = (float*)inputImage.accessPixels(); // Allows the pixels of the input image ot be accessible

    const int kernelSize = 27; // constant integer declaring the kernel size
    int halfKernel = kernelSize / 2; // half kernel benefits bounds checking

    float kernel[kernelSize][kernelSize]; //generating the kernel array that is the size of kernelSize

    float sigma = kernelSize / 2; // value of sigma = kernelsize / 2
    double sum = 0.0f;
    int i; // integer for the for loops below
    int j; // integer for the for loops below

    for(i = 0; i < kernelSize; i++)
    {
        for (j = 0; j < kernelSize; j++)
        {
            kernel[i][j] = Gaussian2D(i - halfKernel, j - halfKernel, sigma); // values within the array =
            sum += kernel[i][j];                                              // Gaussian blur calculation, value of i
            cout << kernel[i][j] << " ";                                      // minus value of half kernel
        }                                                                     // value of j - halfkernel then sigma
        cout << endl;
    }

    for(i = 0; i < kernelSize; i++)
    {
        for (j = 0; j < kernelSize; j++)
        {
            kernel[i][j] = kernel[i][j]/sum; //Values within the array = array divided by the value of sum
        }
        //cout << endl;
    }

    auto startSG = tick_count::now(); // timer
    for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                for(int i = -halfKernel; i <= halfKernel; i++)
                {
                    for (int j = -halfKernel; j <= halfKernel; j++)
                    {
                        if(y + i > 0 && y + i < height && x + j > 0 && x + j < width)
                            outputBuffer[y * width + x] += kernel[j + halfKernel][i + halfKernel] * inputBuffer[(y+i) * width + (x+j)];
                    }
                }
            }
        }
    auto finishSG = tick_count::now(); // timer ends
    /*cout << "Saving Image!! ";
    cout << endl;
    outputImage.convertToType(FREE_IMAGE_TYPE::FIT_BITMAP);
    outputImage.convertTo24Bits();
    outputImage.save("grey_blurred.png");*/

    cout << "...done\n\n";
    cout << "Gaussian Blur Sequential Time: " << (finishSG - startSG).seconds();
    cout << endl;

   auto startPG = tick_count::now();
    parallel_for(blocked_range2d<int, int>(0, height, 0, width), [=](const blocked_range2d<int, int>& r)
    {
        for (auto y = r.rows().begin(); y < r.rows().end(); y++)
        {                                                                                                      //performs the same for loop above
            for (auto x = r.cols().begin(); x < r.cols().end(); x++)                                           //with the addition of utilizing parallel_for
            {                                                                                                  //to execute the code in parallel
                for(int i = -halfKernel; i <= halfKernel; i++)
                {
                    for (int j = -halfKernel; j <= halfKernel; j++)
                    {
                        if(y + i > 0 && y + i < height && x + j > 0 && x + j < width)
                            outputBuffer[y * width + x] += kernel[j + halfKernel][i + halfKernel] * inputBuffer[(y+i) * width + (x+j)];
                    }
                }
            }
        }
    });
    auto finishPG = tick_count::now(); // timer ends
    cout << "Saving Image!! ";
    cout << endl;
    outputImage.convertToType(FREE_IMAGE_TYPE::FIT_BITMAP); //image converts from float to a bitmap image
    outputImage.convertTo24Bits(); //converts it to a 24bit image
    outputImage.save("grey_blurred.png"); //gives the new output image with a specified image name

    cout << "...done\n\n";
    cout << "Gaussian Blur parallel Time: " << (finishPG - startPG).seconds(); // displays the time taken for the parallel code to execute
    cout << endl;
}

void part2Sequential(int sequentialThreshold = 1)
{
    //Setup Input image array
    fipImage input_Image[2];
    input_Image[0].load("../Images/render_1.png");
    input_Image[1].load("../Images/render_2.png");

    unsigned int width = input_Image[0].getWidth();
    unsigned int height = input_Image[1].getHeight();

    // Setup Output image array
    fipImage outputImage;
    outputImage = fipImage(FIT_BITMAP, width, height, 24);

    //total number of pixels
    int totalNumberOfPixels = width*height;

    //float containing the percentage

    //2D Vector to hold the RGB colour data of an image
    vector<vector<RGBQUAD>> rgbValues;
    rgbValues.resize(height, vector<RGBQUAD>(width));


    RGBQUAD rgb;  //FreeImage structure to hold RGB values of a single pixel

    //Absolute difference of images!
    //Extract colour data from image and store it as individual RGBQUAD elements for every pixel
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {// 2 RGB quads to hold RGB values for the 2 given images!
                RGBQUAD rgb1;
                RGBQUAD rgb2;
                input_Image[0].getPixelColor(x, y, &rgb1); //Extract pixel(x,y) colour data and place it in rgb
                input_Image[1].getPixelColor(x, y, &rgb2); //Extract pixel(x,y) colour data and place it in rgb

                //Uniary threshhold to show the differences between the images as white!
                if (abs(rgb1.rgbRed - rgb2.rgbRed) >= sequentialThreshold && abs(rgb1.rgbGreen - rgb2.rgbGreen) >= sequentialThreshold &&
                    abs(rgb1.rgbBlue - rgb2.rgbBlue) >= sequentialThreshold) {
                    rgbValues[y][x].rgbRed = 255;
                    rgbValues[y][x].rgbGreen = 255;
                    rgbValues[y][x].rgbBlue = 255;
                }

            }
        }

    //Place the pixel colour values into output image
    for(int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            outputImage.setPixelColor(x, y, &rgbValues[y][x]);
        }
    }

    //Save the processed image
    outputImage.save("RGB_processed.png");
    //counting the white pixels for the images!
    //TO REVERT ANYTHING FROM PARALLEL TO SEQUENTIAL, CHANGE FOR LOOP VALUES TO 0, Y = Height, x = Width.

    int count = 0;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++) {
            //Gets average of the RGB Pixels
            int average = (rgbValues[y][x].rgbRed + rgbValues[y][x].rgbGreen + rgbValues[y][x].rgbBlue) / 3;
            //if average == 255 (white), then increment count.
            if (average == 255)
                count++;
        }
    }
    cout << endl;
    cout << "Sequential Part 2:";
    cout << endl;
    cout << endl;
    float totalPercentage = (count / float(totalNumberOfPixels)) * 100;
    cout << "Total number of pixels: " << totalNumberOfPixels;
    cout << endl;
    cout << "Number of white pixels: " << count;
    cout << endl;
    cout << "Percentage for white/totalNumberOfPixels: " << totalPercentage;
    cout << endl;

    // random number generator that will generate a random number associated with the width
    int randWidth;
    random_device randGenerator;
    mt19937 mersenne(randGenerator());
    uniform_int_distribution<int> dist(0, width);
    randWidth = dist(mersenne);

    // random number generator that will generate a random number associated with the height
    int randHeight;
    dist = uniform_int_distribution<int>(0, height);
    randHeight = dist(mersenne);

    rgbValues[randHeight][randWidth].rgbRed = 255;
    rgbValues[randHeight][randWidth].rgbGreen = 0;
    rgbValues[randHeight][randWidth].rgbBlue = 0;

    cout << "Red pixel placed: " << randWidth << ", " << randHeight;
    cout << endl;
    //finding the red pixel by using cancellation via parallel programming!
    int identifiedX, identifiedY;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (rgbValues[y][x].rgbRed == 255 && rgbValues[y][x].rgbGreen == 0 &&
                    rgbValues[y][x].rgbBlue == 0) { //cancels all tasks within the group!
                    if (task::self().cancel_group_execution()) {

                        identifiedY = y;
                        identifiedX = x;
                    }
                }
            }
        }
    cout << "Red pixel located: " << identifiedX << ", " << identifiedY;
    cout << endl;
}


// Does this
// threshold - DEFINE
void part2Parallel(int threshold = 1)
{
    //Setup Input image array
    fipImage input_Image[2];
    input_Image[0].load("../Images/render_1.png");
    input_Image[1].load("../Images/render_2.png");

    unsigned int width = input_Image[0].getWidth();
    unsigned int height = input_Image[1].getHeight();

    // Setup Output image array
    fipImage outputImage;
    outputImage = fipImage(FIT_BITMAP, width, height, 24);

    //total number of pixels
    int totalNumberOfPixels = width*height;

    //float containing the percentage

    //2D Vector to hold the RGB colour data of an image
    vector<vector<RGBQUAD>> rgbValues;
    rgbValues.resize(height, vector<RGBQUAD>(width));

    //Absolute difference of images!
    //Extract colour data from image and store it as individual RGBQUAD elements for every pixel
    parallel_for(blocked_range2d<int, int>(0, height, 0, width), [&](
    blocked_range2d < int, int > &range)
    {//blocked range applied to the for loop
    for (int y = range.rows().begin(); y < range.rows().end(); y++) {
        for (int x = range.cols().begin(); x < range.cols().end(); x++) {// 2 RGB quads to hold RGB values for the 2 given images!
            RGBQUAD rgb1; //FreeImage structure to hold RGB values of a single pixel
            RGBQUAD rgb2; //FreeImage structure to hold RGB values of a single pixel
            input_Image[0].getPixelColor(x, y, &rgb1); //Extract pixel(x,y) colour data and place it in rgb
            input_Image[1].getPixelColor(x, y, &rgb2); //Extract pixel(x,y) colour data and place it in rgb

            //threshhold to show the differences between the images as white!
            if (abs(rgb1.rgbRed - rgb2.rgbRed) >= threshold && abs(rgb1.rgbGreen - rgb2.rgbGreen) >= threshold &&
                abs(rgb1.rgbBlue - rgb2.rgbBlue) >= threshold) {
                rgbValues[y][x].rgbRed = 255;
                rgbValues[y][x].rgbGreen = 255;
                rgbValues[y][x].rgbBlue = 255;
            }

        }
    }
});

    //Place the pixel colour values into output image
    for(int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            outputImage.setPixelColor(x, y, &rgbValues[y][x]);
        }
    }

    //Save the processed image
    outputImage.save("RGB_processed.png");
    //counting the white pixels for the images!
    //TO REVERT ANYTHING FROM PARALLEL TO SEQUENTIAL, CHANGE FOR LOOP VALUES TO 0, Y = Height, x = Width.
    int counter = parallel_reduce(blocked_range2d<int, int>(0, height, 0, width), 0, [&](blocked_range2d<int, int> &range, int countWhite) -> int
                                  {

                                      for (int y = range.rows().begin(); y < range.rows().end(); y++) {
                                          for (int x = range.cols().begin(); x < range.cols().end(); x++) {
                                              //Gets average of the RGB Pixels
                                              int average = (rgbValues[y][x].rgbRed + rgbValues[y][x].rgbGreen + rgbValues[y][x].rgbBlue) / 3;
                                              //if average == 255 (white), then increment countWhite.
                                              if (average == 255)
                                                  countWhite++;
                                          }
                                      }


                                  }, [&](int x, int y) -> int { return x + y; }
    );
    cout << "Parallel Part 2:";
    cout << endl;
    cout << endl;
    float totalPercentage = (counter / float(totalNumberOfPixels)) * 100;
    cout << "Total number of pixels: " << totalNumberOfPixels;
    cout << endl;
    cout << "Number of white pixels: " << counter;
    cout << endl;
    cout << "Percentage for white/totalNumberOfPixels: " << totalPercentage;
    cout << endl;

    // random number generator that will generate a random number associated with the width
    int randWidth;
    random_device randGenerator;
    mt19937 mersenne(randGenerator());
    uniform_int_distribution<int> dist(0, width);
    randWidth = dist(mersenne);

    // random number generator that will generate a random number associated with the height
    int randHeight;
    dist = uniform_int_distribution<int>(0, height);
    randHeight = dist(mersenne);

    rgbValues[randHeight][randWidth].rgbRed = 255;
    rgbValues[randHeight][randWidth].rgbGreen = 0;
    rgbValues[randHeight][randWidth].rgbBlue = 0;

    cout << "Red pixel placed: " << randWidth << ", " << randHeight;
    cout << endl;
    //finding the red pixel by using cancellation via parallel programming!
    int identifiedX, identifiedY;
    parallel_for(blocked_range2d<int, int>(0, height, 0, width), [&](
            blocked_range2d < int, int > &range)
    {
        for (int y = range.rows().begin(); y < range.rows().
        end(); y++)      //if rgbValues == 255 for red and 0 for green and blue, assign y & x to IdentifiedX & Y
        {                                                                    //Cancel the group of tasks
            for (int x = range.cols().begin(); x < range.cols().end(); x++)
            {
                if(rgbValues[y][x].rgbRed == 255 && rgbValues[y][x].rgbGreen == 0 && rgbValues[y][x].rgbBlue == 0)
                { //cancels all tasks within the group!
                    if (task::self().cancel_group_execution())
                    {

                        identifiedY = y;
                        identifiedX = x;
                    }
                }
            }
        }
    });
    cout << "Red pixel located: " << identifiedX << ", " << identifiedY;
    cout << endl;
}

int main() {
    int nt = task_scheduler_init::default_num_threads();
    task_scheduler_init T(nt);

    //Part 1 (Greyscale Gaussian blur): -----------DO NOT REMOVE THIS COMMENT----------------------------//
    part1GaussianBlur();


    //Part 2 (Colour image processing): -----------DO NOT REMOVE THIS COMMENT----------------------------//
    auto startP = tick_count::now();
    part2Parallel(1);
    auto finishP = tick_count::now();

    auto startS = tick_count::now();
    part2Sequential(1);
    auto finishS = tick_count::now();
    cout << "Parallel part 2 speed test using tick_count: " << (finishP - startP).seconds();
    cout << endl;
    cout << "Sequential part 2 speed test using tick_count: " << (finishS - startS).seconds();

    return 0;
}