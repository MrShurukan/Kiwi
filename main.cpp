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

int otherX = -1, otherY = -1;

void draw(sf::RenderWindow* window) {
    // Помечаем окно активным в этом потоке
    window->setActive(true);

    while (window->isOpen())
    {
        // clear the window with black color
        window->clear(sf::Color::White);

        // draw everything here...
        // window.draw(...);
        sf::Text text;
        text.setFont(mainFont);
        text.setString("Hello World!");
        text.setCharacterSize(24);
        text.setFillColor(sf::Color::Red);
        text.setStyle(sf::Text::Bold | sf::Text::Italic);
        text.setOrigin(text.getLocalBounds().width / 2, text.getLocalBounds().height / 2);
        text.setPosition(sf::Vector2f(width / 2.0, height / 2.0));

        window->draw(text);

        sf::CircleShape shape(50.0);
        shape.setFillColor(sf::Color(200, 100, 100));
        shape.setOrigin(shape.getRadius(), shape.getRadius());
        shape.setPosition(mouseX, mouseY);
        shape.setPointCount(100);
        window->draw(shape);

        if (otherX != -1 && otherY != -1) {
            shape.setFillColor(sf::Color(100, 200, 100));
            shape.setPosition(otherX, otherY);
            window->draw(shape);
        }

        // end the current frame
        window->display();
    }
}

void loadSound(sf::SoundBuffer& bf, std::string path) {
    if (!bf.loadFromFile(path)) {
        D(std::cout << "Couldn't load sound from '" << path << "'\n");
        exit(1);
    }
}

int main()
{
    D(std::cout << "Kiwi v0.1 (c) IZ\n");
    
    /* Подгрузка звуковых файлов */
    sf::SoundBuffer boomp_buffer;
    sf::Sound boomp;
    sf::SoundBuffer yi_buffer;
    sf::Sound yi;

    if (!mainFont.loadFromFile("BalooBhaina2-Regular.ttf")) {
        D(std::cout << "Couldn't load font\n");
        exit(1);
    }

    loadSound(boomp_buffer, "boomp.wav");
    loadSound(yi_buffer, "yi.wav");

    boomp.setBuffer(boomp_buffer);
    yi.setBuffer(yi_buffer);

    /* Настройка сети */
    sf::TcpSocket socket;
    if (socket.connect("localhost", 25565) != sf::Socket::Done) {
        D(std::cout << "Failed to connect\n");
        exit(1);
    }
    
    width = 800;
    height = 800;

    // Устанавливаем 8-ой уровень сглаживания
    sf::ContextSettings settings;
    settings.antialiasingLevel = 8;

    // Create a window with the same pixel depth as the desktop
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(sf::VideoMode(width, height, desktop.bitsPerPixel), "Kiwi", sf::Style::Titlebar | sf::Style::Close, settings);
    window.setVerticalSyncEnabled(true);
    window.setKeyRepeatEnabled(false);

    // "Отключаем" окно в этом потоке, чтобы его можно было рендерить на другом
    window.setActive(false);
    
    // Отрисовка на другом потоке
    std::thread drawingThread(draw, &window);

    std::atomic<bool> finishThreads = false;
    // Обработка сети на другом потоке
    std::thread networkThread([&yi, &boomp, &finishThreads](sf::TcpSocket* socket) {
        sf::SocketSelector selector;
        selector.add(*socket);
        while (!finishThreads) {
            if (selector.wait(sf::milliseconds(100))) {
                sf::Packet packet;
                if (socket->receive(packet) != sf::Socket::Done) {
                    D(std::cout << "Error while receving or a timeout\n");
                }
                else {
                    std::string msg;
                    packet >> msg;

                    D(std::cout << "Received: " << msg << '\n');

                    if (msg == "y") {
                        yi.play();
                    }
                    else if (msg == "b") {
                        boomp.play();
                    }
                    else if (msg == "coords") {
                        packet >> otherX >> otherY;
                    }
                }
            }
        }
    }, &socket);

    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type) {
            case sf::Event::Closed: {
                sf::Packet packet; packet << "disconnect";
                socket.send(packet);
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
                    
                case sf::Keyboard::S: {
                    D(std::cout << "YI\n");
                    yi.play();

                    sf::Packet packet;
                    packet << "y";

                    socket.send(packet);
                    break;
                }

                case sf::Keyboard::B: {
                    D(std::cout << "BOOMP\n");
                    boomp.play();

                    sf::Packet packet;
                    packet << "b";

                    socket.send(packet);
                    break;
                }

                }
                break;

            case sf::Event::MouseButtonPressed:
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    D(
                        std::cout << "the right button was pressed" << std::endl;
                        std::cout << "mouse x: " << event.mouseButton.x << std::endl;
                        std::cout << "mouse y: " << event.mouseButton.y << std::endl;
                    )
                }
                break;

            case sf::Event::MouseMoved: {
                mouseX = event.mouseMove.x;
                mouseY = event.mouseMove.y;
                // D(std::cout << mouseX << " " << mouseY << '\n');

                sf::Packet packet;
                packet << "coords" << mouseX << mouseY;
                socket.send(packet);

                break;
            }

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
    finishThreads = true;
    networkThread.join();

    return 0;
}