# TCPQuizGame
# TCP-Based Group Quiz Game

This is a network-based multiple-choice quiz game developed in C using TCP sockets. It supports 2–10 players simultaneously, where all participants take the quiz together in real-time. At the end of the quiz, players can see their individual results, including which questions were answered correctly or incorrectly, and the top scorer(s) are displayed to all clients.

---

## Features

- Real-time **multiplayer quiz** (2 to 10 clients)
- **Retry option** for each client (up to 3 times)
- **Correct/Incorrect feedback** after each question
- **Detailed result display** at the end of the quiz
- **Top scorer(s)** shown to all clients

---

## How to Run

###  1. Compile

Use `gcc` to compile both server and client:

```bash
gcc server.c -o server -lpthread
gcc client.c -o client
````

###  2. Start the Server

Run the server on a desired port:

```bash
./server
```

### 3. Start Clients

On separate terminals (or devices), run:

```bash
./client
```

####  Minimum 2 clients are required for the quiz to start.

---

## Gameplay Instructions

1. The first client waits for others to join.
2. When 2 or more clients are connected, the quiz begins.
3. Each client answers the same questions.
4. After all questions:

   * Each client sees a **detailed summary** of correct and incorrect answers.
   * **Top scorer(s)** is displayed to everyone.
5. Clients can **retry the quiz** (up to 3 times) by entering `R`.

---

## Updates & Changes Made

### Updated Version Includes:

* **Retry Functionality**:

  * Clients can retry the quiz up to 3 times independently.
*  **Group Quiz Mode**:

  * Quiz begins only when at least 2 clients are connected.
*  **Results Breakdown**:

  * Each player sees which questions were correct and incorrect.
  * Final score and accuracy feedback.
*  **Top Scorer(s) Displayed**:

  * At the end of the quiz, top performers are shown to all clients.

---

##  License

This project is developed as part of an academic assignment and is open for educational and non-commercial use.


