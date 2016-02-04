#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <iostream>

#define MODE_VIEW 1
#define MODE_EDIT 2

using namespace cv;
using namespace std;

class Box {
public:
    Box();
    Box(const Rect& r);
    Box(const Rect& r, const string& ct);

    Rect rect;
    string content;
};

inline Box::Box(): rect(Rect(0, 0, 0, 0)) {}
inline Box::Box(const Rect& r): rect(r) {}
inline Box::Box(const Rect& r, const string& ct): rect(r), content(ct) {}

Mat img, display;
Point pt1(0, 0);
Point pt2(0, 0);
Rect selectRect(0, 0, 0, 0);
Rect originRect;

bool clicked = false;
bool moving = false;
Box* selected = NULL;
int unitSize = 5;
int mode = MODE_VIEW;
string inputText;

vector<Box> boxes;

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

    for (vector<Box>::iterator it = boxes.begin(); it != boxes.end(); it++) {
        if (it->rect.width > 0 && it->rect.height > 0) {
            if (&(*it) == selected) {
                rectangle(display, it->rect, Scalar(0, 0, 255), 1, 8, 0);
            } else {
                rectangle(display, it->rect, Scalar(0, 255, 0), 1, 8, 0);
            }
        }
    }

    if (!text.empty()) {
        int baseline = 0;
        int fontFace = CV_FONT_HERSHEY_DUPLEX;
        double fontScale = 2;
        int thickness = 3;

        Size textSize = getTextSize(text, fontFace, fontScale,
                                    thickness, &baseline);
        baseline += thickness;

        // center the text
        Point textOrg(min((display.cols - textSize.width)/2, display.cols - textSize.width),
                      (display.rows + textSize.height)/2);

        // baseline
        line(display, textOrg + Point(0, thickness),
             textOrg + Point(textSize.width, thickness),
             Scalar(0, 0, 255));

        // text
        putText(display, text, textOrg, fontFace, fontScale,
                Scalar(0, 255, 0), thickness, 8);
    }

    if (mode == MODE_VIEW || mode == MODE_EDIT) {
        string modeText = "VIEW";
        if (mode == MODE_EDIT) {
            modeText = "EDIT";
        }

        int baseline = 0;
        int fontFace = CV_FONT_HERSHEY_SIMPLEX;
        double fontScale = 1;
        int thickness = 2;

        Size textSize = getTextSize(modeText, fontFace, fontScale,
                                    thickness, &baseline);
        baseline += thickness;
        putText(display, modeText, Point(display.cols - textSize.width - 10, textSize.height + 5),
                fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);
    }

    imshow("ImageDisplay", display);
}

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (mode == MODE_EDIT) {
        return;
    }

    if (clicked) {
        pt2.x = x;
        pt2.y = y;
    }

    switch(event) {
    case CV_EVENT_LBUTTONDOWN:
        clicked = true;
        pt1.x = pt2.x = x;
        pt1.y = pt2.y = y;
        selected = NULL;
        moving = false;
        for (vector<Box>::iterator it = boxes.begin(); it != boxes.end(); it++) {
            if (it->rect.contains(pt1)) {
                originRect = Rect(it->rect);
                selected = &(*it);
                break;
            }
        }
        if (selected) {
            moving = true;
        } else {
            boxes.push_back(Box(Rect(x, y, 1, 1)));
            selected = &boxes.back();
        }
        break;
    case CV_EVENT_LBUTTONUP:
        clicked = false;
        moving = false;
        break;
    case CV_EVENT_MOUSEMOVE:
        if (clicked) {
            int x0 = min(pt1.x, pt2.x);
            int y0 = min(pt1.y, pt2.y);
            if (moving) {
                x0 = originRect.x + pt2.x - pt1.x;
                y0 = originRect.y + pt2.y - pt1.y;
            } else {
                selected->rect.width = max(pt1.x, pt2.x) - x0 + 1;
                selected->rect.height = max(pt1.y, pt2.y) - y0 + 1;
            }
            selected->rect.x = x0;
            selected->rect.y = y0;
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
    if (selected && selected->rect.width > 0 && selected->rect.height > 0) {
        selected->rect.x = min(max(0, selected->rect.x + unitSize * x), img.cols - selected->rect.width);
        selected->rect.y = min(max(0, selected->rect.y + unitSize * y), img.rows - selected->rect.height);
        showImage();
    }
}

void changeSize(int x, int y) {
    if (selected->rect.width > 0 && selected->rect.height > 0) {
        if (selected->rect.width + x * unitSize > 0) {
            selected->rect.width = min(selected->rect.width + x * unitSize, img.cols - selected->rect.x);
        }
        if (selected->rect.height + y * unitSize > 0) {
            selected->rect.height = min(selected->rect.height + y * unitSize, img.rows - selected->rect.y);
        }
        showImage();
    }
}

void remove() {
    for (vector<Box>::iterator it = boxes.begin(); it != boxes.end(); it++) {
        if (&(*it) == selected) {
            boxes.erase(it);
            selected = NULL;
            showImage();
            break;
        }
    }
}

void enterEditMode() {
    if (selected) {
        mode = MODE_EDIT;
        inputText = selected->content;
        showImage(inputText);
    }
}

void leaveEditMode(bool save) {
    if (save) {
        selected->content = inputText;
    }
    inputText = "";
    mode = MODE_VIEW;
    showImage();
}

void help() {
    cout << "======================== Help ========================" << endl;
    cout << "---------------- VIEW MODE ----------------" << endl;
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

    cout << "------> Press 'CTRL-e' to edit content" << endl;
    cout << "------> Press 'CTRL-d' to remove box" << endl << endl;

    cout << "------> Press 'CTRL-n' to go to next image" << endl;
    cout << "------> Press 'CTRL-p' to go to previous image" << endl << endl;

    cout << "------> Press 'h' to get help" << endl;
    cout << "------> Press 'CTRL-s' to save" << endl;
    cout << "------> Press 'CTRL-q' to quit" << endl;
    cout << "-------------------------------------------" << endl << endl;

    cout << "---------------- EDIT MODE ----------------" << endl;
    cout << "------> Press 'ENTER' to confirm" << endl;
    cout << "------> Press 'ESC' to cancel" << endl;
    cout << "------> Press 'DEL' to delete last character" << endl;
    cout << "------> Press 'CTRL-u' to delete all content" << endl;
    cout << "-------------------------------------------" << endl;
    cout << "======================================================" << endl;
}

void handleViewModeKey(int key) {
    switch (key) {
    case 4: // CTRL-d
        remove();
        break;
    case 5: // CTRL-e
        enterEditMode();
        break;
    case 17: // CTRL-q
        exit(0);
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

void handleEditModeKey(int key) {
    string showText = inputText;
    switch (key) {
    case 13: // Carriage Return, CTRL-m
        leaveEditMode(true);
        showText = "";
        break;
    case 21: // CTRL-U
        inputText = "";
        showText = inputText;
        break;
    case 27: // ESC
        leaveEditMode(false);
        showText = "";
        break;
    case 127: // DEL
        inputText = inputText.substr(0, inputText.length() - 1);
        showText = inputText;
        break;
    default:
        if (key >= 32 && key <= 126) {
            inputText += (char)key;
            showText = inputText;
        }
        break;
    }
    showImage(showText);
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
    showImage();

    while (true) {
        // Wait until user press some key
        int key = waitKey(1000);
        if (mode == MODE_VIEW) {
            handleViewModeKey(key);
        } else if (mode == MODE_EDIT) {
            handleEditModeKey(key);
        }
    }

    return 0;
}
