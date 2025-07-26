#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <ctime>

// functie checkt of muis over knop is (hover)
bool isMouseOverButton(const sf::RectangleShape& button, const sf::RenderWindow& window) {
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window)); // muis positie in world coords
    return button.getGlobalBounds().contains(mousePos); // check of muis binnen knop valt
}

class Pipe {
public:
    sf::Sprite topPipe, bottomPipe;
    bool passed = false;

    Pipe(const sf::Texture& texture, float x, float gapY) {
        topPipe.setTexture(texture);
        bottomPipe.setTexture(texture);

        float pipeHeight = static_cast<float>(texture.getSize().y);
        float gapSize = 250.f;

        float topHeight = gapY - gapSize / 2.f;
        float bottomHeight = 720.f - (gapY + gapSize / 2.f);

        constexpr float scaleX = 2.0f; // consistent scale
        float scaleYTop = -topHeight / pipeHeight * scaleX;
        float scaleYBottom = bottomHeight / pipeHeight * scaleX;

        topPipe.setScale(scaleX, scaleYTop);
        topPipe.setPosition(x, gapY - gapSize / 2.f);

        bottomPipe.setScale(scaleX, scaleYBottom);
        bottomPipe.setPosition(x, gapY + gapSize / 2.f);
    }

    void move(float dx) { // pipe naar links verplaatsen
        topPipe.move(dx, 0);
        bottomPipe.move(dx, 0);
    }

    float getX() const { // X positie van pipe (voor score en verwijderen)
        return topPipe.getPosition().x;
    }

    bool isOffScreen() const { // check of pipe uit scherm is (links)
        return getX() + topPipe.getGlobalBounds().width < 0;
    }

    void draw(sf::RenderWindow& window) { // pipe tekenen
        window.draw(topPipe);
        window.draw(bottomPipe);
    }

    bool checkCollision(const sf::FloatRect& birdBounds) const { // check botsing met raketplayer
        return topPipe.getGlobalBounds().intersects(birdBounds) ||
            bottomPipe.getGlobalBounds().intersects(birdBounds);
    }
};

class Bird {
public:
    sf::Sprite sprite;
    sf::Texture texture;
    float velocity = 0;
    float gravity = 1000.0f; // zwaartekracht pixels/s^2
    float flapStrength = -350.0f; // kracht van flap
    sf::Sound flapSound;

    Bird() {
        texture.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\raket.png"); // Trumpraket plaatje
        sprite.setTexture(texture);
        sprite.setPosition(100, 300); // start positie
        sprite.setScale(0.5f, 0.5f); // halve grootte
        sprite.setOrigin(texture.getSize().x / 2, texture.getSize().y / 2); // midden als origine voor roteren en pos
    }

    void flap() { // raket boost omhoog
        velocity = flapStrength;
        flapSound.play(); // boost geluid afspelen
    }

    void update(float dt) { // update positie met delta tijd dt
        velocity += gravity * dt; // snelheid omhoog door zwaartekracht
        sprite.move(0, velocity * dt); // beweeg trumpraket
    }

    void setFlapSound(sf::SoundBuffer& buffer) { // geluid instellen
        flapSound.setBuffer(buffer);
    }

    void reset() { // reset trumpraket naar startpositie
        sprite.setPosition(100, 300);
        velocity = 0;
    }
};

// hitbox voor botsing, kleiner dan sprite zelf voor betere gameplay
sf::FloatRect getBirdHitbox(const sf::Sprite& sprite) {
    sf::FloatRect bounds = sprite.getGlobalBounds();
    return sf::FloatRect(bounds.left + 60, bounds.top + 100, bounds.width - 120, bounds.height - 155);
}

// teken rode hitbox als debug
void drawHitbox(sf::RenderWindow& window, const sf::FloatRect& rect) {
    sf::RectangleShape box;
    box.setPosition(rect.left, rect.top);
    box.setSize({ rect.width, rect.height });
    box.setFillColor(sf::Color(0, 0, 0, 0)); // volledig transparant
    box.setOutlineColor(sf::Color::Red);
    box.setOutlineThickness(2.f);
    window.draw(box);
}

class Game {
private:
    sf::RenderWindow window;
    sf::Texture backgroundTex, menuBackgroundTex, pipeTexture;
    sf::Sprite background, menuBackground;

    Bird bird;
    std::vector<Pipe> pipes;

    sf::Font font;
    sf::Text scoreText, deathText;

    int score = 0, deathCount = 0;
    bool showHitboxes = false, isMenu = true, isGameOver = true, isPaused = false;

    sf::Clock clock;
    float spawnTimer = 0;

    sf::SoundBuffer flapBuffer, scoreBuffer, hitBuffer;
    sf::Sound scoreSound, hitSound;

    sf::Music backgroundMusic;

    sf::Texture jetTexture;
    sf::Sprite jet;
    float jetSpeed = 450.f;
    sf::SoundBuffer jetSoundBuffer;
    sf::Sound jetSound;

    bool jetSoundPlayed = false;
    float jetTimer = 0.f;
    float jetInterval = 8.f;

    sf::RectangleShape startButton, resumeButton, exitButton;
    sf::Text startText, resumeText, exitText;

public:
    Game() : window(sf::VideoMode(1080, 720), "Flappy Bird - SFML 2.6.2") {
        window.setFramerateLimit(60); // max 60 fps

        backgroundTex.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\background.png"); // achtergrond plaatje laden
        background.setTexture(backgroundTex);
        background.setPosition(-825, 0); // startpositie achtergrond
        background.scale(1.5f, 1.0f); // groter maken

        menuBackgroundTex.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\menu_background.png");
        menuBackground.setTexture(menuBackgroundTex);
        menuBackground.setPosition(-825, 0);
        menuBackground.scale(1.5f, 1.0f);

        pipeTexture.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\raket_pipe_blauwe.png");

        font.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\FlappyBirdy.ttf"); // lettertype

        scoreText.setFont(font);
        scoreText.setCharacterSize(36);
        scoreText.setFillColor(sf::Color::White);
        scoreText.setPosition(10, 10); // score linksboven

        deathText.setFont(font);
        deathText.setCharacterSize(24);
        deathText.setFillColor(sf::Color::White);
        deathText.setPosition(10, 50); // deaths onder score

        flapBuffer.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\flap.wav");
        scoreBuffer.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\score.wav");
        hitBuffer.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\TrumpOhnawwh.wav");

        bird.setFlapSound(flapBuffer);
        scoreSound.setBuffer(scoreBuffer);
        hitSound.setBuffer(hitBuffer);

        backgroundMusic.openFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\backgroundMusic.wav");
        backgroundMusic.setLoop(true);
        backgroundMusic.setVolume(40.f);
        backgroundMusic.play();

        jetTexture.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\jet.png");
        jet.setTexture(jetTexture);
        jet.setScale(0.4f, 0.4f);
        jet.setPosition(-300.f, 100.f);

        jetSoundBuffer.loadFromFile("C:\\Users\\User\\Documents\\C++\\Flappy Bird\\FlappyBird_2.6.2\\Assets\\jet_flyby.wav");
        jetSound.setBuffer(jetSoundBuffer);
        jetSound.setVolume(60.f);

        sf::Vector2f buttonSize(300.f, 60.f);
        startButton.setSize(buttonSize);
        startButton.setFillColor(sf::Color(100, 250, 100));
        startButton.setPosition(390.f, 300.f);

        resumeButton.setSize(buttonSize);
        resumeButton.setFillColor(sf::Color(100, 150, 250));
        resumeButton.setPosition(390.f, 300.f);

        exitButton.setSize(buttonSize);
        exitButton.setFillColor(sf::Color(250, 100, 100));
        exitButton.setPosition(390.f, 400.f);

        startText.setFont(font);
        startText.setString("Start Game");
        startText.setCharacterSize(28);
        startText.setFillColor(sf::Color::Black);

        resumeText.setFont(font);
        resumeText.setString("Resume");
        resumeText.setCharacterSize(28);
        resumeText.setFillColor(sf::Color::Black);

        exitText.setFont(font);
        exitText.setString("Exit");
        exitText.setCharacterSize(28);
        exitText.setFillColor(sf::Color::Black);

        srand(static_cast<unsigned>(time(0))); // random seed
    }

    void run() {
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds(); // delta tijd

            handleEvents(); // event handler

            if (!isMenu && !isPaused) update(dt); // update alleen als niet in menu of gepauzeerd

            draw(); // teken alles
        }
    }

private:
    void handleEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space && !isMenu && !isPaused)
                    bird.flap();

                if (event.key.code == sf::Keyboard::P && !isGameOver) {
                    isPaused = true; // pauze aan
                    isMenu = true; // menu aan
                }

                if (event.key.code == sf::Keyboard::H)
                    showHitboxes = !showHitboxes; // toggle hitbox tekenen
            }

            if (event.type == sf::Event::MouseButtonPressed && isMenu) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

                if (startButton.getGlobalBounds().contains(mousePos)) {
                    isMenu = false; // uit menu
                    isPaused = false; // niet pauze
                    if (isGameOver) {
                        resetGame(); // reset spel
                        isGameOver = false; // niet game over meer
                    }
                }

                if (resumeButton.getGlobalBounds().contains(mousePos) && !isGameOver) {
                    isMenu = false;
                    isPaused = false;
                }

                if (exitButton.getGlobalBounds().contains(mousePos)) {
                    window.close(); // sluit spel
                }
            }
        }
    }

    void update(float dt) {
        bird.update(dt); // trumpraket positie updaten

        spawnTimer += dt; // tijd bijhouden voor pipe spawn

        if (spawnTimer >= 2.0f) { // elke 2 sec pipe erbij
            float gapY = 200 + rand() % 250; // random gap positie
            pipes.emplace_back(pipeTexture, 1100.f, gapY); // nieuwe pipe aan rechterkant
            spawnTimer = 0;
        }

        for (auto& pipe : pipes)
            pipe.move(-200 * dt); // pipes naar links bewegen

        for (auto it = pipes.begin(); it != pipes.end();) {
            if (it->isOffScreen()) it = pipes.erase(it); // verwijder pipes uit scherm
            else {
                // als pipe niet gepasseerd en trumpraket is voorbij pipe
                if (!it->passed && it->getX() + 52 < bird.sprite.getPosition().x) {
                    it->passed = true;
                    score++; // score verhogen
                    scoreSound.play(); // score geluid
                }
                // check botsing
                if (it->checkCollision(getBirdHitbox(bird.sprite))) {
                    onDeath(); // dood
                    return; // stop update
                }
                ++it;
            }
        }

        // jet animatie en geluid
        jetTimer += dt;
        if (jetTimer >= jetInterval) {
            jet.move(jetSpeed * dt, 0);
            if (jet.getPosition().x > 0 && !jetSoundPlayed) {
                jetSound.play();
                jetSoundPlayed = true;
            }
            if (jet.getPosition().x > 1200) {
                jet.setPosition(-300.f, 50.f + rand() % 200);
                jetSoundPlayed = false;
                jetTimer = 0.f;
                jetInterval = 6.f + static_cast<float>(rand() % 6);
            }
        }

        // check als player van scherm valt (dood)
        if (bird.sprite.getPosition().y > 700 || bird.sprite.getPosition().y < -100)
            onDeath();

        scoreText.setString("Score: " + std::to_string(score)); // score tekst bijwerken
        deathText.setString("Deaths: " + std::to_string(deathCount)); // deaths tekst bijwerken
    }

    void draw() {
        window.clear(); // scherm schoonmaken
        window.draw(background); // achtergrond tekenen
        window.draw(jet); // jet tekenen

        for (auto& pipe : pipes) {
            pipe.draw(window); // pipe tekenen
            if (showHitboxes) { // als hitboxes aan
                drawHitbox(window, pipe.topPipe.getGlobalBounds()); // teken hitbox top pipe
                drawHitbox(window, pipe.bottomPipe.getGlobalBounds()); // teken hitbox bottom pipe
            }
        }

        window.draw(bird.sprite); // trumpraket tekenen
        if (showHitboxes) // hitbox trumpraket tekenen
            drawHitbox(window, getBirdHitbox(bird.sprite));

        if (isMenu) {
            window.draw(menuBackground); // menu achtergrond

            // dim overlay voor menu
            sf::RectangleShape dim(sf::Vector2f(window.getSize().x, window.getSize().y));
            dim.setFillColor(sf::Color(0, 0, 0, 80)); // semi-transparant zwart
            window.draw(dim);

            // functie om knop te tekenen met hover effect
            auto drawButton = [&](sf::RectangleShape& button, sf::Text& text) {
                if (isMouseOverButton(button, window)) {
                    sf::Color c = button.getFillColor();
                    c.r = std::min(255, c.r + 40);
                    c.g = std::min(255, c.g + 40);
                    c.b = std::min(255, c.b + 40);
                    button.setFillColor(c);
                }
                else {
                    if (&button == &startButton) button.setFillColor(sf::Color(100, 250, 100));
                    else if (&button == &resumeButton) button.setFillColor(sf::Color(100, 150, 250));
                    else if (&button == &exitButton) button.setFillColor(sf::Color(250, 100, 100));
                }

                window.draw(button);

                sf::FloatRect btnBounds = button.getGlobalBounds();
                sf::FloatRect txtBounds = text.getLocalBounds();
                // tekst centreren in knop
                text.setOrigin(txtBounds.left + txtBounds.width / 2.f, txtBounds.top + txtBounds.height / 2.f);
                text.setPosition(btnBounds.left + btnBounds.width / 2.f, btnBounds.top + btnBounds.height / 2.f);
                window.draw(text);
                };

            drawButton(startButton, startText);
            if (!isGameOver) drawButton(resumeButton, resumeText);
            drawButton(exitButton, exitText);
        }

        // schaduw voor score voor leesbaarheid
        sf::Text scoreShadow = scoreText;
        scoreShadow.setFillColor(sf::Color(0, 0, 0, 150));
        scoreShadow.setPosition(scoreText.getPosition() + sf::Vector2f(2, 2));
        window.draw(scoreShadow);
        window.draw(scoreText);

        // schaduw voor deaths
        sf::Text deathShadow = deathText;
        deathShadow.setFillColor(sf::Color(0, 0, 0, 150));
        deathShadow.setPosition(deathText.getPosition() + sf::Vector2f(2, 2));
        window.draw(deathShadow);
        window.draw(deathText);

        window.display(); // scherm updaten
    }

    void onDeath() { // wat er gebeurt als vogel dood gaat
        deathCount++; // deaths tellen
        hitSound.play(); // geluid afspelen
        pipes.clear(); // pipes verwijderen
        bird.reset(); // player resetten
        score = 0; // score reset
        isMenu = true; // terug naar menu
        isGameOver = true; // game over staat aan
        isPaused = false; // niet pauze
    }

    void resetGame() { // reset alles voor nieuwe game
        pipes.clear();
        bird.reset();
        score = 0;
    }
};

int main() {
    Game game;
    game.run(); // start game loop
    return 0;
}
