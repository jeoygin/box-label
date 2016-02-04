#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <iostream>

using namespace cv;
using namespace std;

Mat img, display;
Point pt1(0, 0);
Point pt2(0, 0);
Rect selectRect(0, 0, 0, 0);

bool clicked = false;
int unitSize = 5;

int makedirs(char * path, mode_t mode) {
    struct stat st = {0};

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode) == 0) {
            return -1;
        }
        return 0;
    }

    char subpath[512] = "";
    char * delim = strrchr(path, '/');
    if (delim != NULL) {
        strncat(subpath, path, delim - path);
        makedirs(subpath, mode);
    }
    if (mkdir(path, mode) != 0) {
        return -1;
    }
    return 0;
}

void showImage(string text="") {
    display = img.clone();

    if (selectRect.width > 0 && selectRect.height > 0) {
        rectangle(display, selectRect, Scalar(0, 0, 255), 1, 8, 0);
    }

    if (!text.empty()) {
        int baseline = 0;
        int fontFace = FONT_HERSHEY_SCRIPT_SIMPLEX;
        double fontScale = 2;
        int thickness = 3;

        Size textSize = getTextSize(text, fontFace, fontScale,
                                    thickness, &baseline);
        baseline += thickness;

        // center the text
        Point textOrg((display.cols - textSize.width)/2,
                      (display.rows + textSize.height)/2);

        // baseline
        line(display, textOrg + Point(0, thickness),
             textOrg + Point(textSize.width, thickness),
             Scalar(0, 0, 255));

        // text
        putText(display, text, textOrg, fontFace, fontScale,
                Scalar(0, 255, 0), thickness, 8);
    }

    imshow("ImageDisplay", display);
}

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (clicked) {
        pt2.x = x;
        pt2.y = y;
    }

    switch(event) {
    case CV_EVENT_LBUTTONDOWN:
        clicked = true;
        pt1.x = pt2.x = x;
        pt1.y = pt2.y = y;
        break;
    case CV_EVENT_LBUTTONUP:
        clicked = false;
        break;
    case CV_EVENT_MOUSEMOVE:
        if (clicked) {
            int x0 = min(pt1.x, pt2.x);
            int y0 = min(pt1.y, pt2.y);
            selectRect.x = x0;
            selectRect.y = y0;
            selectRect.width = max(pt1.x, pt2.x) - x0 + 1;
            selectRect.height = max(pt1.y, pt2.y) - y0 + 1;
        }
        break;
    default:
        break;
    }

    if (clicked) {
        showImage();
    }
}

void changeUnitSize(int value) {
    unitSize = max(1, unitSize + value);
    showImage(to_string(unitSize));
}

void move(int x, int y) {
    if (selectRect.width > 0 && selectRect.height > 0) {
        selectRect.x = min(max(0, selectRect.x + unitSize * x), img.cols - selectRect.width);
        selectRect.y = min(max(0, selectRect.y + unitSize * y), img.rows - selectRect.height);
        showImage();
    }
}

void changeSize(int x, int y) {
    if (selectRect.width > 0 && selectRect.height > 0) {
        if (selectRect.width + x * unitSize > 0) {
            selectRect.width = min(selectRect.width + x * unitSize, img.cols - selectRect.x);
        }
        if (selectRect.height + y * unitSize > 0) {
            selectRect.height = min(selectRect.height + y * unitSize, img.rows - selectRect.y);
        }
        showImage();
    }
}

void help() {
    cout << "======================== Help ========================" << endl;
    cout << "------> Press 'i' to move up" << endl;
    cout << "------> Press 'k' to move down" << endl;
    cout << "------> Press 'j' to move right" << endl;
    cout << "------> Press 'l' to move left" << endl << endl;

    cout << "------> Press '^' to grow box height by 1 unit" << endl;
    cout << "------> Press '_' to shrink box height by 1 unit" << endl;
    cout << "------> Press '>' to grow box width by 1 unit" << endl;
    cout << "------> Press '<' to shrink box width by 1 unit" << endl;
    cout << "------> Press '+' to increase the unit size (default: 5)" << endl;
    cout << "------> Press '-' to decrease the unit size (default: 5)" << endl << endl;

    cout << "------> Press 'CTRL+n' to go to next image" << endl;
    cout << "------> Press 'CTRL+p' to go to previous image" << endl << endl;

    cout << "------> Press 'h' to get help" << endl;
    cout << "------> Press 'CTRL+s' to save" << endl;
    cout << "------> Press 'CTRL+q' to quit" << endl;
    cout << "======================================================" << endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        return -1;
    }
    // Read image from file
    img = imread(argv[1]);

    // If fail to read the image
    if (img.empty()) {
        cout << "Error loading the image" << endl;
        return -1;
    }

    help();

    display = img.clone();

    // Create a window
    namedWindow("ImageDisplay", 1);

    // Set the callback function for any mouse event
    setMouseCallback("ImageDisplay", onMouse, NULL);

    // Show the image
    imshow("ImageDisplay", display);

    while (true) {
        // Wait until user press some key
        int key = waitKey(1000);
        switch (key) {
        case 17: // CTRL+q
            return 0;
            break;
        case (int)'i':
            move(0, -1);
            break;
        case (int)'k':
            move(0, 1);
            break;
        case (int)'j':
            move(-1, 0);
            break;
        case (int)'l':
            move(1, 0);
            break;
        case (int)'>':
            changeSize(1, 0);
            break;
        case (int)'<':
            changeSize(-1, 0);
            break;
        case (int)'^':
            changeSize(0, 1);
            break;
        case (int)'_':
            changeSize(0, -1);
            break;
        case (int)'+':
            changeUnitSize(1);
            break;
        case (int)'-':
            changeUnitSize(-1);
            break;
        case (int)'h':
            help();
            break;
        default:
            showImage();
            break;
        }
    }

    return 0;
}
