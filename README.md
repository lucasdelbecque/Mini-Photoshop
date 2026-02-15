# Moteur de Traitement d'Image GPU

Une application de traitement d'image haute performance en temps réel, développée "from scratch" en **C++** et **Modern OpenGL (3.3+)**.

Ce projet démontre l'implémentation d'un pipeline graphique programmable pour effectuer des opérations de traitement du signal (convolutions, filtrage spatial, manipulation colorimétrique) directement sur le GPU via des Fragment Shaders GLSL personnalisés. Il intègre une interface utilisateur graphique (GUI) réactive basée sur **Dear ImGui** pour le contrôle dynamique des paramètres.

## Présentation du Projet

Contrairement aux éditeurs d'images traditionnels basés sur le CPU qui itèrent séquentiellement sur les pixels, ce moteur exploite la puissance de calcul parallèle du GPU. Chaque pixel est traité simultanément via un pipeline graphique personnalisé, permettant des ajustements sans latence de filtres complexes comme le Flou Gaussien ou la Détection de Contours, même sur des images haute résolution.

### Fonctionnalités Clés

* **Filtrage GPU Temps Réel** : Toutes les manipulations d'image se font dans le Fragment Shader.
    * **Flou Gaussien** : Noyau de convolution dynamique avec rayon variable.
    * **Détection de Contours** : Implémentation de type Sobel pour l'extraction de caractéristiques structurelles.
    * **Mosaïque / Pixélisation** : Manipulation des coordonnées UV pour la réduction locale de la résolution spatiale.
    * **Colorimétrie** : Conversion en niveaux de gris (luminance pondérée) et inversion négative.
* **Flux Non-Destructif** : Les filtres peuvent être activés et combinés dynamiquement sans altérer les données de la texture source originale.
* **Architecture Modern OpenGL** : Respect strict du Core Profile (VAO, VBO, EBO). Aucune fonction dépréciée (`glBegin`/`glEnd`) n'a été utilisée.
* **Support High-DPI** : Support natif des écrans Retina/4K avec mise à l'échelle correcte du viewport et des coordonnées UI.
* **Gestion Robuste des Textures** : Implémentation du `GL_CLAMP_TO_EDGE` et du filtrage linéaire pour éviter les artefacts de convolution sur les bords de l'image.

## Stack Technique

* **Langage** : C++17
* **API Graphique** : OpenGL 3.3 Core Profile
* **Langage Shader** : GLSL 330
* **Fenêtrage** : GLFW
* **Librairie GUI** : Dear ImGui
* **Chargement d'Image** : stb_image
* **Système de Build** : CMake

## Architecture

L'application suit une séparation stricte entre l'application hôte (CPU) et le pipeline de rendu (GPU) :

1.  **Initialisation** : L'image générique `input.jpg` est chargée en VRAM sous forme de Texture 2D.
2.  **Boucle d'Application** :
    * **Entrées** : `GLFW` gère les événements fenêtres et souris.
    * **GUI** : `Dear ImGui` effectue le rendu de la barre latérale et capture les paramètres (intensité, rayon, cases à cocher).
    * **Uniforms** : L'hôte C++ envoie ces paramètres au GPU via des variables Uniformes.
3.  **Pipeline de Rendu** :
    * **Vertex Shader** : Transmet la géométrie et les coordonnées de texture.
    * **Fragment Shader** : Exécute les boucles de convolution et la logique de mixage des couleurs par pixel.
4.  **Sortie** : Le résultat est rendu sur un quad plein écran (Full-screen quad).

## Prérequis

* **Compilateur C++** : Clang (macOS), GCC (Linux), ou MSVC (Windows).
* **CMake** : Version 3.10 ou supérieure.
* **GLFW** : Doit être installé sur le système.

### Installation des dépendances (macOS)

    brew install glfw

## Instructions de Compilation

1.  **Cloner le dépôt** :
    
    git clone <url_du_depot>
    cd <dossier_du_depot>

2.  **Préparer le dossier de build** :

    mkdir build
    cd build

3.  **Configurer et Compiler** :

    cmake ..
    make

4.  **Lancer l'application** :
    Assurez-vous qu'une image nommée `input.jpg` est présente dans le dossier de l'exécutable.

    ./MiniPhotoshop

## Utilisation

1.  **Lancement** : L'application s'ouvre avec l'image par défaut chargée.
2.  **Contrôles Latéraux** : Utilisez le panneau inspecteur sur la droite pour activer les filtres.
3.  **Sliders d'Intensité** : Ajustez les curseurs pour modifier la taille du noyau (pour le Flou) ou le facteur de mixage (pour les autres effets).
4.  **Combinaison** : Plusieurs filtres peuvent être actifs simultanément. Le shader les applique dans un ordre logique (Mosaïque -> Flou -> Ajustements colorimétriques).

## Auteur

**Lucas Delbecque**

Développé comme démonstration technique de l'accélération matérielle pour le traitement d'image et la programmation graphique bas niveau.
