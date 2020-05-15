#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <iostream>
#include <math.h>
#include <fstream>
#include <thread>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

#ifdef DEBUG
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

// Количество нот в октаве
#define NoteNumber 12

// Как много мы проскролили наверх (в нотах)
int verticalOffset = NoteNumber * 4; // Сразу скроллим piano roll на четыре октавы наверх
// Как много мы проскролили вправо
int horizontalOffset = 0;


// Позиция вертикального ползунка
int verticalScrollY = -1;
#define VerticalScrollWidth 20
#define VerticalScrollHeight 60

#define HorizontalScrollWidth VerticalScrollHeight
// #define HorizontalScrollHeight VerticalScrollWidth

// Позиция горизонтального ползунка
int horizontalScrollX = -1;

// Выбрал ли пользователь представление через диезы?
bool isSharpsSelected = true;

std::string notes_sharps[NoteNumber] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
std::string notes_flats[NoteNumber] = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

// Поставлена ли нота в этом месте сетки?
// 10 октав
std::atomic<bool> isNoteSet[NoteNumber * 10][32];

// Размер символов слева в пикселях
const int charSize = 24;

// Длина одной ноты в пикселях
int noteWidth = 70;

// Разметка одного диапазона значений на другой
double mapRanges(double value, double input_start, double input_end, double output_start, double output_end) {
    return (value - input_start) * (output_end - output_start) / (input_end - input_start) + output_start;
}

// std::vector<std::thread*> soundThreads;

void createSound(sf::SoundBuffer& buffer, double pitch = 1) {
    std::thread soundThread([buffer, pitch]() {
        sf::Sound sound(buffer);
        sound.setPitch(pitch);

        sound.play();
        // while (sound->getStatus() == sf::Sound::Playing);
        std::this_thread::sleep_for(200ms);

        if (sound.getStatus() == sf::Sound::Playing) {
            for (int i = 100; i >= 0; i--) {
                sound.setVolume(i);
                std::this_thread::sleep_for(5ms);
            }
            sound.stop();
        }
    });

    soundThread.detach();

    // soundThreads.push_back(soundThread);
    //D(std::cout << "SoundThread size: " << soundThreads.size() << '\n');
}

std::chrono::milliseconds tempoToMs(int tempo) {
    return std::chrono::milliseconds(60000 / (tempo));
}

// В режиме проигрывания ли сейчас студия?
std::atomic<bool> isPlaying;
int currentPlayPosition = 0;

// Fix Current Play Position
void fixCPP() {
    if (currentPlayPosition < 0) currentPlayPosition = 0;
    else if (currentPlayPosition >= 32) currentPlayPosition = 31;
}

// Текущий темп в BPM
int tempo = 120;
auto playDelay = tempoToMs(tempo);

/*void play() {
    isPlaying = true;
    while (isPlaying) {
        std::this_thread::sleep_for(playDelay);
        currentPlayPosition++;
        if (currentPlayPosition > 32) currentPlayPosition = 0;
        D(std::cout << "currentPlayPosition: " << currentPlayPosition << '\n');
    }
}*/

void play() {
    isPlaying = true;
}

void pause() {
    isPlaying = false;
}

void stop() {
    isPlaying = false;
    currentPlayPosition = 0;
}

void draw(sf::RenderWindow* window) {
    // Помечаем окно активным в этом потоке
    window->setActive(true);

    while (window->isOpen())
    {
        // Заполняем всё окно серым цветом
        window->clear(sf::Color(100, 100, 100));

        sf::Text noteLabel;
        noteLabel.setFont(mainFont);
        //noteLabel.setOutlineColor(sf::Color::Black);
        //noteLabel.setOutlineThickness(1);
        noteLabel.setCharacterSize(charSize);

        sf::RectangleShape horizontalLine(sf::Vector2f(width - VerticalScrollWidth, 1));
        horizontalLine.setFillColor(sf::Color(50, 50, 50));

        // Учитываем пробел в два пикселя
        int labelAmount = (height / (charSize + 2));
        int notesOnScreenHor = (width - (2 * charSize)) / noteWidth;
        std::string* notes_array = (isSharpsSelected ? notes_sharps : notes_flats);

        sf::RectangleShape noteColoring(sf::Vector2f(2*charSize, charSize));
        sf::RectangleShape cellFilling(sf::Vector2f(noteWidth, charSize + 2));
        cellFilling.setFillColor(sf::Color(100, 200, 100));

        sf::RectangleShape playMarker(sf::Vector2f(2, height - charSize + 2));
        playMarker.setFillColor(sf::Color::White);
            
        for (int i = 0; i < labelAmount; i++) {
            int noteIndex = i + verticalOffset;

            // Закрашиваем квадраты черным или белым
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

            // Пишем название ноты и соответсвующую октаву
            std::string string = notes_array[arrayIndex] + std::to_string(noteIndex / NoteNumber);
            noteLabel.setString(string);
            noteLabel.setPosition(0, height - (i+2) * (charSize + 2) - 5);
            window->draw(noteLabel);

            // Рисуем все видимые ноты на этой строчке
            for (int i = 0 + horizontalOffset; i < notesOnScreenHor + horizontalOffset; i++) {
                if (isNoteSet[noteIndex][i]) {
                    int x = noteWidth * (i - horizontalOffset) + (2 * charSize);
                    cellFilling.setPosition(x, vertPos);
                    noteLabel.setPosition(x + (string.size() == 2 ? 22 : 15), vertPos - 1);
                    window->draw(cellFilling);
                    window->draw(noteLabel);
                }
            }

            horizontalLine.setPosition(0, height - (i+2) * (charSize + 2));
            window->draw(horizontalLine);
        }

        // Вертикальная черта слева
        sf::RectangleShape verticalLine(sf::Vector2f(2, height - (charSize + 2)));
        verticalLine.setPosition(2*charSize, 0);
        window->draw(verticalLine);

        // Сетка для нот
        verticalLine.setSize(sf::Vector2f(1, height - (charSize + 2)));
        verticalLine.setFillColor(sf::Color::Black);
        for (int i = 1 + horizontalOffset; i < notesOnScreenHor + horizontalOffset; i++) {
            verticalLine.setPosition(sf::Vector2f(noteWidth * (i - horizontalOffset) + (2 * charSize), 0));

            if (i % 4 == 0) verticalLine.setSize(sf::Vector2f(2, height - (charSize + 2)));
            else verticalLine.setSize(sf::Vector2f(1, height - (charSize + 2)));

            window->draw(verticalLine);
        }

        // Закрашиваем место под вертикальный скролл
        verticalLine.setFillColor(sf::Color(100, 100, 100));
        verticalLine.setPosition(width - VerticalScrollWidth, 0);
        verticalLine.setSize(sf::Vector2f(VerticalScrollWidth, height));
        window->draw(verticalLine);

        // Вертикальный скролл
        verticalLine.setSize(sf::Vector2f(2, height));
        verticalLine.setFillColor(sf::Color::White);
        // verticalLine.setPosition(width - VerticalScrollWidth, 0);
        window->draw(verticalLine);

        sf::RectangleShape scrollHandle(sf::Vector2f(VerticalScrollWidth, VerticalScrollHeight));
        verticalScrollY = mapRanges(verticalOffset, 0, NoteNumber * 10 - labelAmount - 1, height - VerticalScrollHeight, 0);
        scrollHandle.setPosition(width - VerticalScrollWidth, verticalScrollY);

        window->draw(scrollHandle);

        // Горизонтальный скролл
        horizontalLine.setFillColor(sf::Color(255, 255, 255));
        horizontalLine.setSize(sf::Vector2f(horizontalLine.getSize().x, 2));
        horizontalLine.setPosition(0, height - (charSize + 2));
        
        window->draw(horizontalLine);

        // Реверсим размер вертикальной ручки
        scrollHandle.setSize(sf::Vector2f(HorizontalScrollWidth, charSize+2));
        // До 8 тактов на экране
        horizontalScrollX = mapRanges(horizontalOffset, 0, 32 - notesOnScreenHor, 0, width - HorizontalScrollWidth);
        scrollHandle.setPosition(horizontalScrollX, height - scrollHandle.getSize().y);
        window->draw(scrollHandle);

        // Рисуем маркер проигрывания
        playMarker.setPosition((currentPlayPosition - horizontalOffset) * noteWidth + noteWidth / 2 + 2 * charSize, 0);
        window->draw(playMarker);

        // Конец кадра
        window->display();
    }
}

void loadSound(sf::SoundBuffer& bf, std::string path) {
    if (!bf.loadFromFile(path)) {
        std::cout << "Couldn't load sound from '" << path << "'\n";
        exit(1);
    }
}

int noteToPitch(std::string notename) {
    std::string noteLetter;
    if (notename.size() == 2) noteLetter = notename.substr(0, 1);
    else noteLetter = notename.substr(0, 2);

    int noteIndex = -1;
    for (int i = 0; i < NoteNumber; i++) {
        if (notes_sharps[i] == noteLetter) {
            noteIndex = i;
            break;
        }
    }

    if (noteIndex == -1) return -1;
    char noteNumber = notename[notename.size() - 1];
    if (noteNumber < '0' && noteNumber > '9') return -1;
    std::cout << noteIndex << " " << noteNumber << " " << (noteNumber - '0') << '\n';

    return 12 * (noteNumber - '0') + noteIndex;
}

std::string loadedFileName = "null";
std::string loadedInstrumentName = "piano_c4.wav";
int defaultInstrumentPitch = NoteNumber * 5;

void sendFile(std::ifstream& file, sf::TcpSocket& socket) {
    file.seekg(0, std::ios::beg);
    char Buffer[1024];
    while (file.read(Buffer, sizeof(Buffer)))
    {
        socket.send(Buffer, sizeof(Buffer));
    }
    // Отправляем то, что меньше 1024
    socket.send(Buffer, file.gcount());

    socket.send("\xFF\xFF\xFF", 3);
}

const std::string ip = "localhost";
const unsigned short port = 8888;

int main(int argc, char** argv) {
    std::cout << "Kiwi v1.0-beta (c) IZ\n";

    sf::SoundBuffer instrumentBuffer; instrumentBuffer.loadFromFile(loadedInstrumentName);

    if (argc > 1) {
        if (strcmp(argv[1], "--uploadingMode") == 0 || strcmp(argv[1], "-u") == 0) {
            char choice;
            std::cout << "Welcome to uploading mode!\n";
            std::wcout << "Connecting to server.";
            sf::TcpSocket socket;
            while (socket.connect(ip, port) != sf::Socket::Done) {
                std::cout << ".";
                std::this_thread::sleep_for(1s);
            }
            std::cout << "\nSuccess!\n";
            
            while (true) {
                std::cout << "1 - upload .wav as an intrument; 2 - upload .kiwi to share with friends\nChoice: ";
                std::cin >> choice;
                std::string fileName;
                switch (choice) {
                case '1':
                    system("cls");
                    std::cout << "(Here is a local directory sorted by .wav):\n";
                    system("dir /B | find \".wav\"");
                    std::cout << '\n';
                    while (true) {
                        std::cout << "Enter a path (full/relative) to a file (type 'break' to exit): ";
                        std::cin >> fileName;
                        if (fileName == "break") break;
                        // std::cout << fileName << '\n';

                        std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);
                        if (!file) {
                            std::cout << "Couldn't read file correctly\n";
                            continue;
                        }

                        sf::Packet packet;
                        packet << "uploadWav" << fileName;
                        socket.send(packet);

                        sendFile(file, socket);
                        packet.clear();
                        socket.receive(packet);
                        std::string response;
                        packet >> response;
                        
                        if (response == "done") {
                            std::cout << "Success!\n";
                        }
                        else {
                            std::cout << "Something went wrong...\n";
                        }

                        break;
                    }

                    break;
                    
                case '2':
                    system("cls");
                    std::cout << "(Here is a local directory sorted by .kiwi):\n";
                    system("dir /B | find \".kiwi\"");
                    std::cout << '\n';
                    while (true) {
                        std::cout << "Enter a path (full/relative) to a file (type 'break' to exit): ";
                        std::cin >> fileName;
                        if (fileName == "break") break;
                        //std::cout << fileName << '\n';

                        std::ifstream file(fileName, std::ios::in | std::ios::binary | std::ios::ate);
                        if (!file) {
                            std::cout << "Couldn't read file correctly\n";
                            continue;
                        }

                        sf::Packet packet;
                        packet << "uploadKiwi";
                        socket.send(packet);

                        sendFile(file, socket);

                        packet.clear();
                        socket.receive(packet);
                        std::string id;
                        packet >> id;
                        std::cout << "Upload complete, here is the ID of your file: " << id << '\n';
                        std::cout << "(You should save this somewhere)\n";

                        break;
                    }

                    break;

                default:
                    std::cout << "Incorrect option\n";
                    break;
                }
            }
        }
        if (strcmp(argv[1], "--downloadingMode") == 0 || strcmp(argv[1], "-d") == 0) {
            char choice;
            std::cout << "Welcome to downloading mode!\n";  
            while (true) {
                std::cout << "1 - download a new instrument; 2 - enter a code to download .kiwi file\nChoice: ";
                std::cin >> choice;
                switch (choice) {
                case '1':
                    break;

                case '2':
                    break;

                default:
                    std::cout << "Incorrect option\n";
                    break;
                }
            }
        }
        else {
            for (int i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-f") == 0) {
                    std::cout << "File argument is provided\n";
                    if (i + 1 >= argc) {
                        std::cout << "Usage: Kiwi.exe -f *filename*\n";
                        exit(1);
                    }
                    std::ifstream saveFile(argv[i+1]);
                    if (!saveFile) {
                        std::cout << "Couldn't open selected file.\n";
                        exit(1);
                    }
                    saveFile.read((char*)isNoteSet, sizeof(std::atomic<bool>[NoteNumber * 10][32]));
                    loadedFileName = argv[1];
                    std::cout << "Loaded selected file\n\n";
                }
                else if (strcmp(argv[i], "-i") == 0) {
                    std::cout << "Instrument replacement is provided\n";
                    if (i + 1 >= argc) {
                        std::cout << "Usage: Kiwi.exe -i *filename*\n";
                        exit(1);
                    }
                    std::string name = argv[i + 1];
                    if (!instrumentBuffer.loadFromFile(name)) {
                        std::cout << "Couldn't load instrument from file.\n";
                        exit(1);
                    }
                    std::cout << "Loaded selected instrument\n";
                    name = name.substr(0, name.find_last_of("."));
                    std::cout << "Looking for " << name << ".default\n";
                    std::ifstream defaultFile(name + ".default");
                    if (!defaultFile) {
                        std::cout << "Coudln't open (using C4 unless -n is provided)\n\n";
                    }
                    else {
                        std::string note;
                        defaultFile >> note;
                        int pitch = noteToPitch(note);
                        if (pitch == -1) {
                            std::cout << "Note doesn't seem to be correctly formatted (are you using sharps?)\n";
                            exit(1);
                        }
                        defaultInstrumentPitch = pitch;
                    }
                    defaultFile.close();
                }
                else if (strcmp(argv[i], "-n") == 0) {
                    std::cout << "Changing default instrument tone\n";
                    if (i + 1 >= argc) {
                        std::cout << "Usage: Kiwi.exe -n *NOTE*\n(Examples: -t C4; -t A#6)\n";
                    }
                    int pitch = noteToPitch(argv[i+1]);
                    if (pitch == -1) {
                        std::cout << "Note doesn't seem to be correctly formatted (are you using sharps?)\n";
                        exit(1);
                    }
                    std::cout << "Changed default intrument note to " << argv[i + 1] << " (" << pitch << ")\n\n";
                    defaultInstrumentPitch = pitch;
                }
                else if (strcmp(argv[i], "-t") == 0) {
                    std::cout << "Changing default tempo\n";
                    if (i + 1 >= argc) {
                        std::cout << "Usage: Kiwi.exe -t *BPM*\n";
                        exit(1);
                    }
                    tempo = std::stoi(argv[i + 1]);
                    playDelay = tempoToMs(tempo);
                    std::cout << "Changed default tempo to " << tempo << "\n\n";
                }
                
            }
        }
    }

    // На отдельном потоке обрабатываем активные звуки
    /*std::thread soundManagmentThread([] {
        while (!programIsShuttingDown) {
            for (int i = soundThreads.size() - 1; i >= 0; i--) {
                soundThreads[i]->join();
                delete soundThreads[i];
                soundThreads.erase(soundThreads.begin() + i);
            }
        }
    });*/

    // Выделяем поток для обработки задержки между проигрыванием
    std::thread playThread([&instrumentBuffer]() {
        while (!programIsShuttingDown) {
            while (!isPlaying);

            // Сыграть все звуки в данной колонке
            for (int i = 0; i < NoteNumber * 10; i++) {
                if (isNoteSet[i][currentPlayPosition])
                    createSound(instrumentBuffer, std::pow(twelvesRootOf2, i - defaultInstrumentPitch));
            }

            std::this_thread::sleep_for(playDelay);

            if (!isPlaying) continue;

            currentPlayPosition++;
            if (currentPlayPosition >= 32) currentPlayPosition = 0;
            D(std::cout << "currentPlayPosition: " << currentPlayPosition << '\n');
        }
    });
   

    if (!mainFont.loadFromFile("BalooBhaina2-Regular.ttf")) {
        std::cout << "Couldn't load font (BalooBhaina2-Regular.ttf). Are you sure it's in your directory?\n";
        exit(1);
    }
    
    width = 1600;
    height = 773;

    // Устанавливаем 8-ой уровень сглаживания
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    // Создаём окно с такой же глубиной пикселей, как и текущие настройки монитора
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    // Запрещаем изменение размера окна
    sf::RenderWindow window(sf::VideoMode(width, height, desktop.bitsPerPixel), "Kiwi", sf::Style::Titlebar | sf::Style::Close, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);

    // "Отключаем" окно в этом потоке, чтобы его можно было рендерить на другом
    window.setActive(false);
    
    // Отрисовка на другом потоке
    std::thread drawingThread(draw, &window);

    // Для изменения курсора
    // sf::Cursor cursor;

    bool once = true;

    // Пока окно открыто - работает основной поток
    while (window.isOpen())
    {
        // Проверяем все ивенты с последней итерации цикла
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) {
            case sf::Event::Closed: { 
                window.close();
                break;
            }

            /*case sf::Event::Resized: {
                D(std::cout << "New Width: " << event.size.width << '\n';
                std::cout << "New Height: " << event.size.height << '\n');

                width = event.size.width;
                height = event.size.height;

                // Меняем размер внутреннего буфера
                sf::FloatRect visibleArea(0, 0, width, height);
                window.setView(sf::View(visibleArea));

                break;
            }*/

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
                        // Ctrl + R - очистить сетку
                        for (int i = 0; i < NoteNumber * 9; i++) {
                            for (int j = 0; j < 32; j++) {
                                isNoteSet[i][j] = false;
                            }
                        }
                    }
                    break;

                case sf::Keyboard::S:
                    if (event.key.control) {
                        // Ctrl + S - сохранить
                        D(std::cout << "Saving...\n");
                        std::string saveName = "save" + std::to_string(std::time(nullptr)) + ".kiwi";
                        D(std::cout << "Writing to " << saveName << "\n(last_save.kiwi was overwritten)\n");
                        std::ofstream saveFile(saveName);
                        saveFile.write((char*)isNoteSet, sizeof(std::atomic<bool>[NoteNumber * 10][32]));
                        saveFile.close();
                        system("del last_save.kiwi");
                        system(("copy "+saveName+" last_save.kiwi /y").c_str());
                    }
                    break;

                case sf::Keyboard::O:
                    if (event.key.control) {
                        // Ctrl + O - открыть
                        std::cout << "Opening...\n";
                        std::ifstream saveFile("last_save.kiwi");
                        if (saveFile) {
                            std::cout << "Done!\n";
                            saveFile.read((char*)isNoteSet, sizeof(std::atomic<bool>[NoteNumber * 10][32]));
                            loadedFileName = "last_save.kiwi";
                        }
                        else {
                            std::cout << "Couldn't open file.\n";
                        }
                    }
                    break;

                case sf::Keyboard::Left:
                    currentPlayPosition--;
                    fixCPP();
                    break;

                case sf::Keyboard::Right:
                    currentPlayPosition++;
                    fixCPP();
                    break;

                case sf::Keyboard::Up:
                    currentPlayPosition += 4;
                    fixCPP();
                    break;

                case sf::Keyboard::Down:
                    currentPlayPosition -= 4;
                    fixCPP();
                    break;

                case sf::Keyboard::Space:
                    if (!event.key.control) {
                        if (!isPlaying) {
                            play();
                        }
                        else {
                            pause();
                        }
                    }
                    else {
                        stop();
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

                    // Количество подписей нот на экране
                    // int labelsOnScreen = (height / (charSize + 2)) + 1;
                    // Какую ноту относительно низа экрана мы нажимаем?
                    int noteClickedIndex = (height - event.mouseButton.y) / (charSize + 2) - 1;
                    // -1 если мы попали на нижнюю строчку с горизонтальным скролом
                    if (noteClickedIndex >= 0) {
                        int noteAblsoluteIndex = noteClickedIndex + verticalOffset;
                        // Какая это нота?
                        D(std::cout << notes_flats[noteAblsoluteIndex % NoteNumber] << noteAblsoluteIndex / NoteNumber << '\n');
                        // previewSound.setPitch(std::pow(twelvesRootOf2, ablsoluteIndex - NoteNumber * 2));

                        // Если мы нажали на названия нот
                        if (event.mouseButton.x < 2 * charSize) {
                            // previewSound.play();
                            createSound(instrumentBuffer, std::pow(twelvesRootOf2, noteAblsoluteIndex - defaultInstrumentPitch));
                        }
                        // Если мы нажали на рабочую поверхность
                        else if (event.mouseButton.x < (width - VerticalScrollWidth)) {
                            // int rowsOnScreen = (width - (2 * charSize)) / noteWidth;
                            int cellClickedIndex = (event.mouseButton.x - 2 * charSize) / noteWidth;
                            int cellAbsoluteIndex = cellClickedIndex + horizontalOffset;
                            D(std::cout << cellAbsoluteIndex << '\n');

                            isNoteSet[noteAblsoluteIndex][cellAbsoluteIndex] = !isNoteSet[noteAblsoluteIndex][cellAbsoluteIndex];
                            // Сыграть превью, если ноту поставили (если не проигрываем)
                            if (isNoteSet[noteAblsoluteIndex][cellAbsoluteIndex] && !isPlaying)
                                createSound(instrumentBuffer, std::pow(twelvesRootOf2, noteAblsoluteIndex - defaultInstrumentPitch));
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
                        // Допускаем только 8 тактов на экране за проект
                        int notesOnScreen = (width - (2 * charSize)) / noteWidth;
                        if (horizontalOffset > 32 - notesOnScreen) horizontalOffset = 32 - notesOnScreen;
                    }
                    else {
                        verticalOffset += event.mouseWheelScroll.delta;

                        if (verticalOffset < 0) verticalOffset = 0;
                        // Учитываем пробел в два пикселя
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
            // Если мы нажали на названия нот
            /*if (mouseX < 2 * charSize) {
                piano.play();
            }*/
            mouseIsPressed = true;
        }
    }

    programIsShuttingDown = true;
    // Убеждаемся, что потоки тоже завершились перед завершением программы
    drawingThread.join();
    //soundManagmentThread.join();
    stop();
    playThread.join();

    return 0;
}