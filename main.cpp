#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <math.h>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

#ifdef DEBUG
#include <iostream>
#define D(x) x
#else
#define D(x) ;
#endif

bool programIsShuttingDown = false;

sf::Font mainFont;

int mouseX, mouseY;
int width, height;

const double twelvesRootOf2 = std::pow(2, 1.0 / 12);

bool mouseIsPressed = false;
bool pMouseIsPressed = false;

#define NoteNumber 12

// ��� ����� �� ����������� ������ (� �����)
int verticalOffset = NoteNumber * 2; // ����� �������� piano roll �� ��� ������ ������
// ��� ����� �� ����������� ������
int horizontalOffset = 0;


// ������� ������������� ��������
int verticalScrollY = -1;
#define VerticalScrollWidth 20
#define VerticalScrollHeight 60

#define HorizontalScrollWidth VerticalScrollHeight
// #define HorizontalScrollHeight VerticalScrollWidth

// ������� ��������������� ��������
int horizontalScrollX = -1;

// ������ �� ������������ ������������� ����� �����?
bool isSharpsSelected = true;

std::string notes_sharps[NoteNumber] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
std::string notes_flats[NoteNumber] = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

// ���������� �� ���� � ���� ����� �����?
// 10 �����
bool isNoteSet[NoteNumber * 10][32];

// ������ �������� ����� � ��������
const int charSize = 24;

// ����� ����� ���� � ��������
int noteWidth = 70;

// �������� ������ ��������� �������� �� ������
double mapRanges(double value, double input_start, double input_end, double output_start, double output_end) {
    return (value - input_start) * (output_end - output_start) / (input_end - input_start) + output_start;
}

std::vector<std::thread*> soundThreads;

void createSound(sf::SoundBuffer* buffer, double pitch = 1) {
    std::thread* soundThread = new std::thread([buffer, pitch]() {
        sf::Sound* sound = new sf::Sound(*buffer);
        sound->setPitch(pitch);

        sound->play();
        while (sound->getStatus() == sf::Sound::Playing);
        delete sound;
    });

    soundThreads.push_back(soundThread);
}

void draw(sf::RenderWindow* window) {
    // �������� ���� �������� � ���� ������
    window->setActive(true);

    while (window->isOpen())
    {
        // ��������� �� ���� ����� ������
        window->clear(sf::Color(100, 100, 100));

        sf::Text noteLabel;
        noteLabel.setFont(mainFont);
        //noteLabel.setOutlineColor(sf::Color::Black);
        //noteLabel.setOutlineThickness(1);
        noteLabel.setCharacterSize(charSize);

        sf::RectangleShape horizontalLine(sf::Vector2f(width - VerticalScrollWidth, 1));
        horizontalLine.setFillColor(sf::Color(50, 50, 50));

        // ��������� ������ � ��� �������
        int labelAmount = (height / (charSize + 2));
        int notesOnScreenHor = (width - (2 * charSize)) / noteWidth;
        std::string* notes_array = (isSharpsSelected ? notes_sharps : notes_flats);

        sf::RectangleShape noteColoring(sf::Vector2f(2*charSize, charSize));
        sf::RectangleShape cellFilling(sf::Vector2f(noteWidth, charSize + 2));
        cellFilling.setFillColor(sf::Color(100, 200, 100));
            
        for (int i = 0; i < labelAmount; i++) {
            int noteIndex = i + verticalOffset;

            // ����������� �������� ������ ��� �����
            int arrayIndex = noteIndex % NoteNumber;
            if (notes_array[arrayIndex].find("#") != -1 || notes_array[arrayIndex].find("b") != -1) {
                noteColoring.setFillColor(sf::Color::Black);
                noteColoring.setSize(sf::Vector2f(2 * charSize, charSize / 1.5));
                noteLabel.setFillColor(sf::Color::White);
            }
            else {
                noteColoring.setFillColor(sf::Color::White);
                noteColoring.setSize(sf::Vector2f(2 * charSize, charSize));
                noteLabel.setFillColor(sf::Color::Black);
            }
            int vertPos = height - (i + 2) * (charSize + 2);
            noteColoring.setPosition(0, vertPos);
            window->draw(noteColoring);

            // ����� �������� ���� � �������������� ������
            noteLabel.setString(notes_array[arrayIndex] + std::to_string(noteIndex/NoteNumber));
            noteLabel.setPosition(0, height - (i+2) * (charSize + 2) - 5);
            window->draw(noteLabel);

            // ������ ��� ������� ���� �� ���� �������
            for (int i = 0 + horizontalOffset; i < notesOnScreenHor + horizontalOffset; i++) {
                if (isNoteSet[noteIndex][i]) {
                    cellFilling.setPosition(noteWidth * (i - horizontalOffset) + (2 * charSize), vertPos);
                    window->draw(cellFilling);
                }
            }

            horizontalLine.setPosition(0, height - (i+2) * (charSize + 2));
            window->draw(horizontalLine);
        }

        // ������������ ����� �����
        sf::RectangleShape verticalLine(sf::Vector2f(2, height - (charSize + 2)));
        verticalLine.setPosition(2*charSize, 0);
        window->draw(verticalLine);

        // ����� ��� ���
        verticalLine.setSize(sf::Vector2f(1, height - (charSize + 2)));
        verticalLine.setFillColor(sf::Color::Black);
        for (int i = 1 + horizontalOffset; i < notesOnScreenHor + horizontalOffset; i++) {
            verticalLine.setPosition(sf::Vector2f(noteWidth * (i - horizontalOffset) + (2 * charSize), 0));

            if (i % 4 == 0) verticalLine.setSize(sf::Vector2f(2, height - (charSize + 2)));
            else verticalLine.setSize(sf::Vector2f(1, height - (charSize + 2)));

            window->draw(verticalLine);
        }

        // ����������� ����� ��� ������������ ������
        verticalLine.setFillColor(sf::Color(100, 100, 100));
        verticalLine.setPosition(width - VerticalScrollWidth, 0);
        verticalLine.setSize(sf::Vector2f(VerticalScrollWidth, height));
        window->draw(verticalLine);

        // ������������ ������
        verticalLine.setSize(sf::Vector2f(2, height));
        verticalLine.setFillColor(sf::Color::White);
        // verticalLine.setPosition(width - VerticalScrollWidth, 0);
        window->draw(verticalLine);

        sf::RectangleShape scrollHandle(sf::Vector2f(VerticalScrollWidth, VerticalScrollHeight));
        verticalScrollY = mapRanges(verticalOffset, 0, NoteNumber * 10 - labelAmount - 1, height - VerticalScrollHeight, 0);
        scrollHandle.setPosition(width - VerticalScrollWidth, verticalScrollY);

        window->draw(scrollHandle);

        // �������������� ������
        horizontalLine.setFillColor(sf::Color(255, 255, 255));
        horizontalLine.setSize(sf::Vector2f(horizontalLine.getSize().x, 2));
        horizontalLine.setPosition(0, height - (charSize + 2));
        
        window->draw(horizontalLine);

        // �������� ������ ������������ �����
        scrollHandle.setSize(sf::Vector2f(HorizontalScrollWidth, charSize+2));
        // �� 8 ������ �� ������
        horizontalScrollX = mapRanges(horizontalOffset, 0, 32 - notesOnScreenHor, 0, width - HorizontalScrollWidth);
        scrollHandle.setPosition(horizontalScrollX, height - scrollHandle.getSize().y);
        window->draw(scrollHandle);

        // ����� �����
        window->display();
    }
}

void loadSound(sf::SoundBuffer& bf, std::string path) {
    if (!bf.loadFromFile(path)) {
        D(std::cout << "Couldn't load sound from '" << path << "'\n");
        exit(1);
    }
}

int main() {
    D(std::cout << "Kiwi v0.1 (c) IZ\n");

    // �� ��������� ������ ������������ �������� �����
    std::thread soundManagmentThread([] {
        while (!programIsShuttingDown) {
            for (int i = soundThreads.size() - 1; i >= 0; i--) {
                soundThreads[i]->join();
                delete soundThreads[i];
                soundThreads.erase(soundThreads.begin() + i);
            }
        }
    });
    
    sf::SoundBuffer pianoBuffer; pianoBuffer.loadFromFile("piano_c4.wav");
    sf::Sound previewSound(pianoBuffer);

    if (!mainFont.loadFromFile("BalooBhaina2-Regular.ttf")) {
        D(std::cout << "Couldn't load font\n");
        exit(1);
    }
    
    width = 1600;
    height = 773;

    // ������������� 8-�� ������� �����������
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    // ������ ���� � ����� �� �������� ��������, ��� � ������� ��������� ��������
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    // ��������� ��������� ������� ����
    sf::RenderWindow window(sf::VideoMode(width, height, desktop.bitsPerPixel), "Kiwi", sf::Style::Titlebar | sf::Style::Close, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);

    // "���������" ���� � ���� ������, ����� ��� ����� ���� ��������� �� ������
    window.setActive(false);
    
    // ��������� �� ������ ������
    std::thread drawingThread(draw, &window);

    // ��� ��������� �������
    // sf::Cursor cursor;

    // ���� ���� ������� - �������� �������� �����
    while (window.isOpen())
    {
        // ��������� ��� ������ � ��������� �������� �����
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) {
            case sf::Event::Closed: { 
                window.close();
                break;
            }

            case sf::Event::Resized: {
                D(std::cout << "New Width: " << event.size.width << '\n';
                std::cout << "New Height: " << event.size.height << '\n');

                width = event.size.width;
                height = event.size.height;

                // ������ ������ ����������� ������
                sf::FloatRect visibleArea(0, 0, width, height);
                window.setView(sf::View(visibleArea));

                break;
            }

            case sf::Event::KeyPressed:
                switch(event.key.code) {
                case sf::Keyboard::Escape:
                    D(std::cout << "the escape key was pressed" << std::endl;
                      std::cout << "control:" << event.key.control << std::endl;
                      std::cout << "alt:" << event.key.alt << std::endl;
                      std::cout << "shift:" << event.key.shift << std::endl;
                      std::cout << "system:" << event.key.system << std::endl);
                    break;
                
                case sf::Keyboard::R:
                    if (event.key.control) {
                        // Ctrl + R - �������� �����
                        for (int i = 0; i < NoteNumber * 9; i++) {
                            for (int j = 0; j < 32; j++) {
                                isNoteSet[i][j] = false;
                            }
                        }
                    }
                    break;
                }
                break;

            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    D(
                        std::cout << "the left button was pressed" << std::endl;
                        /*std::cout << "mouse x: " << event.mouseButton.x << std::endl;
                        std::cout << "mouse y: " << event.mouseButton.y << std::endl;*/
                    )

                    // ���������� �������� ��� �� ������
                    // int labelsOnScreen = (height / (charSize + 2)) + 1;
                    // ����� ���� ������������ ���� ������ �� ��������?
                    int noteClickedIndex = (height - event.mouseButton.y) / (charSize + 2) - 1;
                    // -1 ���� �� ������ �� ������ ������� � �������������� �������
                    if (noteClickedIndex >= 0) {
                        int noteAblsoluteIndex = noteClickedIndex + verticalOffset;
                        // ����� ��� ����?
                        D(std::cout << notes_flats[noteAblsoluteIndex % NoteNumber] << noteAblsoluteIndex / NoteNumber << '\n');
                        // previewSound.setPitch(std::pow(twelvesRootOf2, ablsoluteIndex - NoteNumber * 2));

                        // ���� �� ������ �� �������� ���
                        if (event.mouseButton.x < 2 * charSize) {
                            // previewSound.play();
                            createSound(&pianoBuffer, std::pow(twelvesRootOf2, noteAblsoluteIndex - NoteNumber * 2));
                        }
                        // ���� �� ������ �� ������� �����������
                        else if (event.mouseButton.x < (width - VerticalScrollWidth)) {
                            // int rowsOnScreen = (width - (2 * charSize)) / noteWidth;
                            int cellClickedIndex = (event.mouseButton.x - 2 * charSize) / noteWidth;
                            int cellAbsoluteIndex = cellClickedIndex + horizontalOffset;
                            D(std::cout << cellAbsoluteIndex << '\n');

                            isNoteSet[noteAblsoluteIndex][cellAbsoluteIndex] = !isNoteSet[noteAblsoluteIndex][cellAbsoluteIndex];
                        }
                    }
                }
                break;

            case sf::Event::MouseButtonReleased:
                if (event.mouseButton.button == sf::Mouse::Left) {
                    /*if (previewSound.Playing) {
                        for (int i = 100; i >= 0; i--) {
                            previewSound.setVolume(i);
                            std::this_thread::sleep_for(5ms);
                        }
                        previewSound.stop();
                        previewSound.setVolume(100);
                    }*/
                }
                break;

            case sf::Event::MouseMoved:
                mouseX = event.mouseMove.x;
                mouseY = event.mouseMove.y;
                // D(std::cout << mouseX << " " << mouseY << '\n');

                break;

            case sf::Event::MouseWheelScrolled:
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    /*D(std::cout << "Scrolling vertically: ");
                    D(std::cout << "Delta: " << event.mouseWheelScroll.delta << '\n');*/

                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
                        horizontalOffset += event.mouseWheelScroll.delta;

                        if (horizontalOffset < 0) horizontalOffset = 0;
                        // ��������� ������ 8 ������ �� ������ �� ������
                        int notesOnScreen = (width - (2 * charSize)) / noteWidth;
                        if (horizontalOffset > 32 - notesOnScreen) horizontalOffset = 32 - notesOnScreen;
                    }
                    else {
                        verticalOffset += event.mouseWheelScroll.delta;

                        if (verticalOffset < 0) verticalOffset = 0;
                        // ��������� ������ � ��� �������
                        int labelN = (height / (charSize + 2)) + 1;
                        // D(std::cout << labelN << '\n');
                        if (verticalOffset + labelN > NoteNumber * 10 + 1) verticalOffset = NoteNumber * 10 + 1 - labelN;
                    }
                }
                break;

            default:
                break;
            }
        }

        // if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) {
        //     D(std::cout << "Playing Z");
        // }

        pMouseIsPressed = mouseIsPressed;
        if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
            // ���� �� ������ �� �������� ���
            /*if (mouseX < 2 * charSize) {
                piano.play();
            }*/
            mouseIsPressed = true;
        }
    }

    programIsShuttingDown = true;
    // ����������, ��� ������ ���� ����������� ����� ����������� ���������
    drawingThread.join();
    soundManagmentThread.join();

    return 0;
}