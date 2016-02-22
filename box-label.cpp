#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>
#include <iostream>
#include <fstream>

#if defined(_WIN32) || defined(__CYGWIN__)
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif

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
string displayWindowName = "ImageDisplay";

string imageListPath;
string workDir;
string boxDir;

bool imageLoaded = false;
bool clicked = false;
bool moving = false;
Box* selected = NULL;
int unitSize = 5;
int mode = MODE_VIEW;
string inputText;
int curImageIdx;
int editPos;

vector<string> images;
vector<Box> boxes;

int makedirs(const char * path, mode_t mode) {
    struct stat st = {0};

    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode) == 0) {
            return -1;
        }
        return 0;
    }

    char subpath[512] = "";
    char * delim = strrchr(path, PATH_SEPARATOR);
    if (delim != NULL) {
        strncat(subpath, path, delim - path);
        makedirs(subpath, mode);
    }
    if (mkdir(path, mode) != 0) {
        return -1;
    }
    return 0;
}

void showImage(string text="", int textPos = -1, double fontScale = 1) {
    display = img.clone();

    for (vector<Box>::iterator it = boxes.begin(); it != boxes.end(); it++) {
        if (it->rect.width > 0 && it->rect.height > 0) {
            Scalar color;
            int thickness = 1;
            if (&(*it) == selected) {
                if (!it->content.empty()) {
                    thickness = 2;
                }
                color = Scalar(0, 0, 255);
            } else if (!it->content.empty()) {
                color = Scalar(0, 255, 255);
            } else {
                color = Scalar(0, 255, 0);
            }
            rectangle(display, it->rect, color, thickness, 8, 0);
        }
    }

    if (!text.empty()) {
        int baseline = 0;
        int fontFace = CV_FONT_HERSHEY_DUPLEX;
        int thickness = 2;

        string formerText, latterText;
        bool showPos = false;
        if (textPos >= 0) {
            formerText = text.substr(0, textPos);
            latterText = text.substr(textPos);
            showPos = true;
        } else {
            formerText = text;
        }

        Size formerTextSize = getTextSize(formerText, fontFace, fontScale,
                                          thickness, &baseline);
        Size latterTextSize = getTextSize(latterText, fontFace, fontScale,
                                          thickness, &baseline);
        int x = 0, y = (display.rows + max(formerTextSize.height, latterTextSize.height))/2;
        int width = formerTextSize.width + latterTextSize.width;
        if (formerTextSize.width > display.cols) {
            x = display.cols - formerTextSize.width;
        } else if (width < display.cols) {
            x = (display.cols - width)/2;
        }
        baseline += thickness;

        // center the text
        Point textOrg(x, y);

        // baseline
        line(display, textOrg + Point(0, thickness),
             textOrg + Point(width, thickness),
             Scalar(0, 0, 255));

        // text
        putText(display, formerText, textOrg, fontFace, fontScale,
                Scalar(0, 255, 0), thickness, 8);
        if (showPos) {
            line(display, Point(x + formerTextSize.width, y),
                 Point(x + formerTextSize.width, y - max(formerTextSize.height, latterTextSize.height)),
                 Scalar(0, 0, 255), 1);
        }
        putText(display, latterText, textOrg + Point(formerTextSize.width, 0),
                fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);
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

    imshow(displayWindowName, display);
}

void changeUnitSize(int value) {
    unitSize = max(1, unitSize + value);
    showImage(to_string(unitSize), -1, 2);
}

void move(int x, int y) {
    if (selected && selected->rect.width > 0 && selected->rect.height > 0) {
        selected->rect.x = min(max(0, selected->rect.x + unitSize * x), img.cols - selected->rect.width);
        selected->rect.y = min(max(0, selected->rect.y + unitSize * y), img.rows - selected->rect.height);
        showImage();
    }
}

void changeSize(int x, int y) {
    if (selected && selected->rect.width > 0 && selected->rect.height > 0) {
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
        editPos = inputText.size();
        showImage(inputText, editPos);
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

bool loadImage(int idx, Mat& img) {
    if (idx < 0 || idx >= images.size()) {
        return false;
    }
    string name = images.at(idx);
    cout << "Loading the image '" << name << "' ";

    // Read image from file
    Mat newimg = imread(workDir + PATH_SEPARATOR + name);
    imageLoaded = false;

    // If fail to read the image
    if (newimg.empty()) {
        cout << "[FAIL]" << endl;
        return false;
    } else {
        cout << "[DONE]" << endl;
    }

    boxes.clear();
    clicked = false;
    moving = false;
    selected = NULL;
    imageLoaded = true;

    img = newimg;
    string boxfile = boxDir + PATH_SEPARATOR + name + "_" +
        to_string(newimg.cols) + "x" + to_string(newimg.rows) + ".box";

    ifstream boxifs(boxfile);
    if (boxifs.is_open()) {
        cout << "Loading the box of image '" << boxfile << "'..." << endl;
        string box;
        while (getline(boxifs, box)) {
            stringstream ss(box);
            vector<string> elems;
            string item, buf;
            while (getline(ss, item, '\t')) {
                elems.push_back(item);
                if (buf.length() > 0) {
                    buf.append("::");
                }
                buf.append(item);
            }
            cout << "    " << buf;
            if (elems.size() < 4) {
                cout << " [FAIL]" << endl;
            } else {
                Rect rect(stoi(elems.at(0)), stoi(elems.at(1)),
                          stoi(elems.at(2)), stoi(elems.at(3)));
                string content;
                if (elems.size() >= 5) {
                    content = elems.at(4);
                }
                boxes.push_back(Box(rect, content));
                cout << " [DONE]" << endl;
            }

        }
    }

    return true;
}

bool saveImage() {
    if (!imageLoaded) {
        return false;
    }

    string name = images.at(curImageIdx);
    string boxfile = boxDir + PATH_SEPARATOR  + name + "_" +
        to_string(img.cols) + "x" + to_string(img.rows) + ".box";
    cout << "Saving the box of image '" << boxfile << "' ";
    if (name.rfind(PATH_SEPARATOR) != std::string::npos) {
        if (makedirs(boxfile.substr(0, boxfile.rfind(PATH_SEPARATOR)).c_str(), 0755) != 0) {
            cout << "[FAIL]" << endl;
            return false;
        }
    }

    ofstream boxofs(boxfile);
    if (boxofs.is_open()) {
        for (vector<Box>::iterator it = boxes.begin(); it != boxes.end(); it++) {
            if (it->rect.width > 1 && it->rect.height > 1) {
                boxofs << it->rect.x << '\t' << it->rect.y << '\t';
                boxofs << it->rect.width << '\t' << it->rect.height << '\t';
                boxofs << it->content << '\n';
            } else {
                boxes.erase(it);
            }
        }
        boxofs.close();
        cout << "[DONE]" << endl;
        return true;
    } else {
        cout << "[FAIL]" << endl;
        return false;
    }
}

bool nextImage() {
    saveImage();
    while (curImageIdx + 1 < images.size()) {
        ++curImageIdx;
        if (loadImage(curImageIdx, img)) {
            showImage(images.at(curImageIdx));
            return true;
        }
    }
    return false;
}

bool previousImage() {
    saveImage();
    while (curImageIdx > 0) {
        --curImageIdx;
        if (loadImage(curImageIdx, img)) {
            showImage(images.at(curImageIdx));
            return true;
        }
    }
    return false;
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
    cout << "------> Press 'DEL' to delete one char backward" << endl;
    cout << "------> Press 'CTRL-a' to move cursor to head" << endl;
    cout << "------> Press 'CTRL-b' to move cursor backward" << endl;
    cout << "------> Press 'CTRL-d' to delete one char forward" << endl;
    cout << "------> Press 'CTRL-e' to move cursor to tail" << endl;
    cout << "------> Press 'CTRL-f' to move cursor forward" << endl;
    cout << "------> Press 'CTRL-k' to delete all from cursor to tail" << endl;
    cout << "------> Press 'CTRL-u' to delete all from cursor to head" << endl;
    cout << "-------------------------------------------" << endl;
    cout << "======================================================" << endl;
}

void handleViewModeKey(int key) {
    switch (key) {
    case 4: // CTRL-d
        remove();
        saveImage();
        break;
    case 5: // CTRL-e
        enterEditMode();
        break;
    case 14: // CTRL-n
        nextImage();
        break;
    case 16: // CTRL-p
        previousImage();
        break;
    case 17: // CTRL-q
        saveImage();
        exit(0);
        break;
    case 19: // CTRL-s
        saveImage();
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
    case 1: // CTRL-a
        editPos = 0;
        break;
    case 2: // CTRL-b
        editPos = max(0, editPos - 1);
        break;
    case 4: // CTRL-d
        inputText = inputText.substr(0, editPos) + inputText.substr(editPos + 1);
        showText = inputText;
        editPos = max(0, editPos - 1);
        break;
    case 5: // CTRL-e
        editPos = inputText.length();
        break;
    case 6: // CTRL-f
        editPos = min(editPos + 1, (int)inputText.length());
        break;
    case 11: // CTRL-k
        inputText = inputText.substr(0, editPos);
        showText = inputText;
        break;
    case 13: // Carriage Return, CTRL-m
        leaveEditMode(true);
        showText = "";
        break;
    case 21: // CTRL-u
        inputText = inputText.substr(editPos);
        showText = inputText;
        editPos = 0;
        break;
    case 27: // ESC
        leaveEditMode(false);
        showText = "";
        break;
    case 127: // DEL
        inputText = inputText.substr(0, editPos - 1) + inputText.substr(editPos);
        showText = inputText;
        editPos = max(0, editPos - 1);
        break;
    default:
        if (key >= 32 && key <= 126) {
            inputText = inputText.substr(0, editPos) + (char)key + inputText.substr(editPos);
            showText = inputText;
            editPos++;
        }
        break;
    }
    if (mode == MODE_EDIT && showText.empty()) {
        showImage("Press any key...");
    } else {
        showImage(showText, editPos);
    }
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
        if (selected && (selected->rect.width <= 1 || selected->rect.height <= 1)) {
            remove();
            selected = NULL;
        }
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

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "Usage: box-label image_list" << endl;
        return -1;
    }

    imageListPath = string(argv[1]);
    ifstream imageList(imageListPath);
    cout << "Loading image list '" << imageListPath << "' ";
    if (imageList.is_open()) {
        string name;
        while (imageList >> name) {
            images.push_back(name);
        }
        imageList.close();
        cout << "[DONE] " << images.size() << " images" << endl;
    } else {
        cout << "[FAIL] Unable to open file " << endl;
        return -1;
    }

    string::size_type lastSepPos = imageListPath.rfind(PATH_SEPARATOR);
    if (lastSepPos != string::npos) {
        workDir = imageListPath.substr(0, lastSepPos);
    } else {
        workDir = ".";
    }
    cout << "Work Dir: " << workDir << endl;

    // Create box dir
    boxDir = workDir + PATH_SEPARATOR + "box";
    cout << "Creating box dir '" << boxDir << "'";
    if (makedirs(boxDir.c_str(), 0755) == 0) {
        cout << " [DONE]" << endl;
    } else {
        cout << " [FAIL]" << endl;
        return -1;
    }

    help();

    // Create a window
    namedWindow(displayWindowName, 1);
    // Set the callback function for any mouse event
    setMouseCallback(displayWindowName, onMouse, NULL);

    // Load the first image
    curImageIdx = -1;
    if (!nextImage()) {
        return -1;
    }

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
