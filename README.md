# Flashcards by Ethan (v1.2.1)

**A clean, modern, and distraction-free flashcard + quiz app.**

![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-orange.svg)
![License](https://img.shields.io/badge/License-GPLv3-9C1C1C.svg)
![Release](https://img.shields.io/github/v/release/ethan-mccall/flashcards-by-ethan?color=4CAF50)

---

## ✨ Features

- **Rich Organization** - Folders, subfolders, and decks with full drag & drop support
- **Two Study Modes** - Classic flashcard mode + Multiple Choice
- **Mastery System** - Per-card mastery tracking with beautiful radial progress indicators
- **Daily Streak** - Built-in streak counter to keep you motivated
- **Home Dashboard** - Quick overview of your library, stats, and recent decks
- **Keyboard Friendly** - Full keyboard navigation and shortcuts
- **Modern UI** - Clean dark theme designed for long study sessions
- **Export / Import** - Backup and restore your entire library

---

| Home Page |
|----------|
| ![Overview](images/screenshot-home-page.png) |

| Deck Editor |
|-------------|
| ![Deck Editor](images/screenshot-deck-page.png) |

| Flashcard Quiz |
|----------------|
| ![Flashcard Mode](images/screenshot-flashcard-quiz.png) |

| Multiple Choice Quiz |
|----------------------|
| ![Quiz Mode](images/screenshot-multi-choice-quiz.png) |

---

## 📥 Downloads & Installation

### Debian / Ubuntu

Download the latest `.deb` from the [Releases page](https://github.com/ethan-mccall/flashcards-by-ethan/releases) and run:
```bash
#Install
sudo dpkg -i flashcards-by-ethan_*.deb
sudo apt install -f

# Update
wget https://github.com/ethan-mccall/flashcards-by-ethan/releases/latest/download/flashcards-by-ethan_1.2.1_amd64.deb
sudo dpkg -i flashcards-by-ethan_*.deb
sudo apt install -f

# Uninstall
sudo dpkg -r flashcards-by-ethan
# or fully remove config files too:
sudo dpkg --purge flashcards-by-ethan
```

#### Snap
```bash
# Install / Update
sudo snap install flashcards-by-ethan
# or
sudo snap refresh flashcards-by-ethan

# Uninstall
sudo snap remove flashcards-by-ethan
# or fully purge:
sudo snap remove --purge flashcards-by-ethan
```
