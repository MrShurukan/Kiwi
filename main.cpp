#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <atomic>

#ifdef DEBUG
#include <iostream>
#define D(x) x
#else
#define D(x) ;
#endif

sf::Font mainFont;

int mouseX, mouseY;
int width, height;

#define NoteNumber 12

// Сразу скроллим piano roll на две октавы наверх
int verticalOffset = NoteNumber * 2;

// Позиция вертикального ползунка
int verticalScrollY = -1;
#define VerticalScrollWidth 20
#define VerticalScrollHeight 60

// Позиция горизонтального ползунка
int horizontalScrollX = -1;

// Выбрал ли пользователь представление через диезы?
bool isSharpsSelected = false;

std::string notes_sharps[NoteNumber] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};
std::string notes_flats[NoteNumber] = {
    "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"
};

// Размер символов слева в пикселях
const int charSize = 24;

// Разметка одного диапазона значений на другой
double mapRanges(double value, double input_start, double input_end, double output_start, double output_end) {
    return (value - input_start) * (output_end - output_start) / (input_end - input_start) + output_start;
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
        noteLabel.setOutlineColor(sf::Color::Black);
        noteLabel.setOutlineThickness(1);
        noteLabel.setCharacterSize(charSize);

        sf::RectangleShape horizontalLine(sf::Vector2f(width - VerticalScrollWidth, 1));
        horizontalLine.setFillColor(sf::Color(50, 50, 50));

        // Учитываем пробел в два пикселя
        int labelN = (height / (charSize + 2));
            
        std::string* notes_array = (isSharpsSelected ? notes_sharps : notes_flats);

        for (int i = 0; i < labelN; i++) {
            int noteIndex = i + verticalOffset;

            // Пишем название ноты и соответсвующую октаву
            noteLabel.setString(notes_array[noteIndex % NoteNumber] + std::to_string(noteIndex/NoteNumber));
            noteLabel.setPosition(0, height - (i+2) * (charSize + 2));
            window->draw(noteLabel);

            horizontalLine.setPosition(0, height - (i+2) * (charSize + 2));
            window->draw(horizontalLine);
        }

        sf::RectangleShape verticalLine(sf::Vector2f(2, height - (charSize + 2)));
        verticalLine.setPosition(2*charSize, 0);
        window->draw(verticalLine);

        // Вертикальный скролл
        verticalLine.setSize(sf::Vector2f(2, height));
        verticalLine.setPosition(width - VerticalScrollWidth, 0);
        window->draw(verticalLine);

        sf::RectangleShape scrollHandle(sf::Vector2f(VerticalScrollWidth, VerticalScrollHeight));
        verticalScrollY = mapRanges(verticalOffset, 0, NoteNumber * 10 - labelN, height - VerticalScrollHeight, 0);
        scrollHandle.setPosition(width - VerticalScrollWidth, verticalScrollY);

        window->draw(scrollHandle);

        // Горизонтальный скролл
        horizontalLine.setFillColor(sf::Color(255, 255, 255));
        horizontalLine.setSize(sf::Vector2f(horizontalLine.getSize().x, 2));
        horizontalLine.setPosition(0, height - (charSize + 2));
        
        window->draw(horizontalLine);

        // Реверсим размер вертикальной ручки
        scrollHandle.setSize(sf::Vector2f(VerticalScrollHeight, charSize+2));
        // horizontalScrollX = 0;
        scrollHandle.setPosition(horizontalScrollX, height - scrollHandle.getSize().y);
        window->draw(scrollHandle);

        // Конец кадра
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

    if (!mainFont.loadFromFile("BalooBhaina2-Regular.ttf")) {
        D(std::cout << "Couldn't load font\n");
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

            case sf::Event::Resized: {
                D(std::cout << "New Width: " << event.size.width << '\n';
                std::cout << "New Height: " << event.size.height << '\n');

                width = event.size.width;
                height = event.size.height;

                // Меняем размер внутреннего буфера
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
                    int labelsOnScreen = (height / (charSize + 2)) + 1;
                    // Какую ноту относительно низа экрана мы нажимаем?
                    int clickedIndex = (height - event.mouseButton.y) / (charSize + 2) - 1;
                    if (clickedIndex >= 0) {
                        int ablsoluteIndex = clickedIndex + verticalOffset;
                        // Какая это нота?
                        D(std::cout << notes_flats[ablsoluteIndex % NoteNumber] << ablsoluteIndex / NoteNumber << '\n');
                    }
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
                        horizontalScrollX += event.mouseWheelScroll.delta;

                        if (horizontalScrollX < 0) horizontalScrollX = 0;
                        if (horizontalScrollX > width) horizontalScrollX = width;
                    }
                    else {
                        verticalOffset += event.mouseWheelScroll.delta;

                        if (verticalOffset < 0) verticalOffset = 0;
                        // Учитываем пробел в два пикселя
                        int labelN = (height / (charSize + 2)) + 1;
                        // D(std::cout << labelN << '\n');
                        if (verticalOffset + labelN > NoteNumber * 10) verticalOffset = NoteNumber * 10 - labelN;
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
    }

    // Убеждаемся, что поток отрисовки тоже завершился перед завершением программы
    drawingThread.join();

    return 0;
}