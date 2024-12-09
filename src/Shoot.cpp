#include <SFML/Graphics.hpp>
#include <iostream>

class Player {
public:
    Player()
        : position(500, 500), angle(45), array(sf::Quads, 4) {
        // Coordenadas relativas al centro del jugador
        array[0].position = sf::Vector2f(-15, -15);
        array[1].position = sf::Vector2f(-15, 15);
        array[2].position = sf::Vector2f(15, 15);
        array[3].position = sf::Vector2f(15, -15);

        // Asignar colores para cada vértice (opcional)
        for (size_t i = 0; i < array.getVertexCount(); i++) {
            array[i].color = sf::Color::White;
        }
    }

    void Draw(sf::RenderWindow& window) {
        // Crear una transformación para la posición y rotación
        sf::Transform transform;
        transform.translate(position).rotate(angle);

        // Dibujar el vértice con la transformación aplicada
        window.draw(array, transform);
    }

    sf::Vector2f position;
    float angle;

private:
    sf::VertexArray array;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Shooting Project",
        sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;
    Player player;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event e{};
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                window.close();
            }
        }

        window.clear(sf::Color::Black); // Limpiar la ventana con color negro

        // Dibujar el jugador
        player.Draw(window);

        window.display(); // Mostrar todo lo dibujado
    }
}
