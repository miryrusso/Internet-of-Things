/***
 * This project simulates a traffic light intersection where the lights communicate in pairs using a Mesh network.
 * The solution is based on a master and a slave for each pair, and to use it, one simply needs to change
 * the initial configuration of the users.
 * Authors: Mirian Russo, Luca Strano, Giulio Ursino, Gaia Cigna e Eleonora
 */
#include <Arduino.h>
#include <WiFi.h>
#include "painlessmesh.h"
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <Update.h>
#include <list>
#include <string>

#define RED_LED 3
#define YELLOW_LED 4
#define GREEN_LED 0

#define DELAY_3SECONDS 3000
#define DELAY_1_5SECONDS 1500

#define MESH_PREFIX "SEM_AFORO"
#define MESH_PASSWORD "Password"
#define MESH_PORT 5555
#define NUM_CONNECTIONS 4

// Utente 1 => SEM0
// #define IDSEM 0 // da 0 a 3
// #define GROUPSEM 0 //0 per il primo gruppo, 1 per il secondo
// #define PARTNERSEM 1 //partner del semaforo
// #define GROUPMASTER true
// #define OTHERGROUPMASTERID 2

// Utente 2  => SEM1
// #define IDSEM 1 // da 0 a 3
// #define GROUPSEM 0 //0 per il primo gruppo, 1 per il secondo
// #define PARTNERSEM 0 //partner del semaforo
// #define GROUPMASTER false
// #define OTHERGROUPMASTERID 2

// Utente 3 => SEM2
// #define IDSEM 2      // da 0 a 3
// #define GROUPSEM 1   // 0 per il primo gruppo, 1 per il secondo
// #define PARTNERSEM 3 // partner del semaforo
// #define GROUPMASTER true
// #define OTHERGROUPMASTERID 0

// Utente 4 => SEM3
#define IDSEM 3      // da 0 a 3
#define GROUPSEM 1   // 0 per il primo gruppo, 1 per il secondo
#define PARTNERSEM 2 // partner del semaforo
#define GROUPMASTER false
#define OTHERGROUPMASTERID 0

#define EVERYONE -1

#define GREEN_TIME 4000
#define YELLOW_TIME 2000
#define RED_TIME 2000
#define FLASHING_TIME 750

#define JSONSIZE 200

typedef enum
{
  WAITING_CONNECT,
  INIT,
  OPERATIVE,
  DISCONNECTED // disconessione in corso
} connection_state;

connection_state conn_state = WAITING_CONNECT;
bool first_op_loop = true;

typedef enum
{
  CHANGE_STATE_CMD,
  CHANGED_STATE_CMD,
  IS_GROUP_RED_CMD, // messaggio utilizzato per testare se il proprio gruppo è rosso
  IS_RED_CMD,       // messaggio di risposta a IS_GROUP_RED: lo slave risponde si o no
  ON_CONNECT_CMD,
  ON_DISCONNECT_CMD
} command;

typedef enum
{
  IDLE, // IDLE durante uno stato
  SWITCH_STATE,
  RED,
  WAITING_RED, // si aspetta che l'altro gruppo diventi ROSSO
  YELLOW,
  GREEN,
  FLASHING
} sem_state;
sem_state state = IDLE;
sem_state last_state = IDLE;
sem_state next_state = IDLE;

painlessMesh mesh;
Scheduler sched; // scheduler azioni semaforo

String jsonMsg; // messaggio da inviare

std::list<uint32_t> nodeList;
bool connected = false;

void sendCommandMsg(command cmd, int to, bool is_red = false); // mandare un json con comando ad un nodo

void callbackOnDroppedConnection()
{
  sendCommandMsg(ON_DISCONNECT_CMD, EVERYONE);
}

void callbackConnection();
Task taskConnection(TASK_MILLISECOND * 500, TASK_FOREVER, &callbackConnection);

void callbackConnection()
{
  sendCommandMsg(ON_CONNECT_CMD, EVERYONE); // si manda ON_CONNECTION A TUTTI
}

void handle_semaphore();
Task taskSemaphore(TASK_MILLISECOND * 1, TASK_FOREVER, &handle_semaphore);

void callbackSendMsg();
void check_connections();

Task taskSendMsg(TASK_MILLISECOND * 1, 1, &callbackSendMsg);

// Task per verificare che il numero di connessioni è insufficente.
Task taskCheckConnections(TASK_SECOND * 1, TASK_FOREVER, &check_connections);

void check_connections()
{
  uint32_t conn_nodes_num = mesh.getNodeList(true).size();
  // Serial.printf("connessioni: %d\n", conn_nodes_num);

  if (conn_nodes_num != NUM_CONNECTIONS && conn_state != WAITING_CONNECT)
  {
    Serial.println("CONNECTION LOST: RESTARTING \n");
    nodeList.clear();
    nodeList.push_back(IDSEM);
    conn_state = DISCONNECTED;
  }
}

void callbackSendMsg()
{
  // Serial.println("SENDING JSON MESSAGE: ");
  // Serial.println(jsonMsg);
  mesh.sendBroadcast(jsonMsg);
  taskSendMsg.setIterations(1);
  taskSendMsg.disable();
}

void sendMsg()
{
  taskSendMsg.enable();
}

bool isNodeInList(int node)
{
  for (const int &el : nodeList)
  {
    if (el == node)
    {
      return true;
    }
  }
  return false;
}

void callbackReceiveMsg(uint32_t from, String &msg)
{
  StaticJsonDocument<JSONSIZE> jsonRcv;
  DeserializationError error = deserializeJson(jsonRcv, msg);
  if (error)
  {
    Serial.println("ERROR PARSING RECEIVED JSON MESSAGE");
    return;
  }
  // se il messaggio non è diretto a questo nodo, scarta
  int to = jsonRcv["to"].as<int>();
  if (to != IDSEM && to != EVERYONE)
  {
    return;
  }

  command cmd = jsonRcv["command"].as<command>();
  int sender = jsonRcv["sender"].as<int>();

  switch (cmd)
  {
  case CHANGE_STATE_CMD:
    // il GROUPMASTER invia il comando di cambiare stato => il nodo deve cambiare stato
    // e successivamente comunicarlo al GROUPMASTER
    if (state != SWITCH_STATE && state != WAITING_RED)
    {
      Serial.println("RECEIVED CHANGE_STATE COMMAND - NOT IN SWITCH STATE OR WAITING RED, SKIPPING");
      break; // cambia soltanto se hai terminato lo stato precedente
    }
    Serial.println("RECEIVED CHANGE_STATE COMMAND - CHANGING STATE AND NOTIFING GROUP MASTER");
    last_state = state;
    sendCommandMsg(CHANGED_STATE_CMD, sender); // ack
    state = jsonRcv["next_state"].as<sem_state>();
    taskSemaphore.forceNextIteration();
    break;

  case CHANGED_STATE_CMD:
    // il partner ha ricevuto il comando di cambiare stato: cambia stato pure il nodo corrente
    Serial.println("RECEIVED CHANGED_STATE COMMAND - CHANGING STATE");
    last_state = state;
    state = next_state;
    taskSemaphore.forceNextIteration();
    break;

  case IS_GROUP_RED_CMD: // questo messaggio è ricevuto solo dallo slave
  {
    Serial.printf("RECEIVED IS_GROUP_RED COMMAND, RED STATE: %i\n", (last_state == RED));
    bool answ = (last_state == RED && state == SWITCH_STATE);
    if (answ)
    {
      last_state = state;
      state = WAITING_RED; // lo slave può passare direttamente a WAITING_RED
      next_state = GREEN;
    }
    sendCommandMsg(IS_RED_CMD, sender, answ);
    break;
  }

  // questo messaggio è ricevuto solo dal master in due casi:
  //   1. Dallo slave dello stesso gruppo per indicare che lo slave è rosso
  //   2. Dal master dell'altro gruppo per indicare che il gruppo è rosso
  case IS_RED_CMD:
  {
    Serial.println("RECEIVED IS_RED COMMAND");
    bool answ = jsonRcv["is_red"].as<bool>();
    // questo messaggio viene ricevuto quando il master è in stato SWITCH_STATE dopo stato RED
    // quindi basta controllare la risposta dello slave, non il proprio stato
    if (answ && sender == PARTNERSEM)
    {
      Serial.println("SLAVE IS RED: SENDING IS_RED COMMAND TO OTHER GROUP");
      // contattiamo l'altro gruppo per comunicare che il gruppo corrente è rosso
      sendCommandMsg(IS_RED_CMD, OTHERGROUPMASTERID, answ);
      last_state = state;
      state = WAITING_RED;
      next_state = GREEN;
    }
    else if (answ && sender == OTHERGROUPMASTERID && state == WAITING_RED)
    {
      // questo gruppo deve diventare verde
      Serial.println("OTHER GROUP IS RED: SWITCHING TO GREEN");
      last_state = state;
      state = SWITCH_STATE; // per sincronizzare il cambio
      next_state = GREEN;
      taskSemaphore.forceNextIteration();
    }
    break;
  }

  case ON_CONNECT_CMD:
    if (!isNodeInList(sender))
    {
      Serial.printf("ON_CONNECT COMMAND RECEIVED FROM SEM%d\n", sender);
      nodeList.push_back(sender);
    }
    break;

  case ON_DISCONNECT_CMD:
    Serial.printf("ON_DISCONNECT COMMAND RECEIVED FROM SEM%d\n", sender);
    if (isNodeInList(sender))
    {
      nodeList.remove(sender);
      // TODO STACCA CONNESSIONE E INIZIA CASO DI WAITING CONNECTION
    }
    break;
  }
}

void ledOnLedsOff(uint8_t high, uint8_t low1, uint8_t low2, uint16_t del)
{
  digitalWrite(high, HIGH);
  digitalWrite(low1, LOW);
  digitalWrite(low2, LOW);
  delay(del);
}

void redOn(uint16_t del)
{
  Serial.printf("Red light on for %.2f seconds\n", (float)del / 1000);
  ledOnLedsOff(RED_LED, YELLOW_LED, GREEN_LED, del);
}

void yellowOn(uint16_t del)
{
  Serial.printf("Yellow light on for %.2f seconds\n", (float)del / 1000);
  ledOnLedsOff(YELLOW_LED, RED_LED, GREEN_LED, del);
}

void greenOn(uint16_t del)
{
  Serial.printf("Green light on for %.2f seconds\n", (float)del / 1000);
  ledOnLedsOff(GREEN_LED, RED_LED, YELLOW_LED, del);
}

void init_mesh()
{
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &sched, MESH_PORT);
  mesh.onReceive(callbackReceiveMsg);
}

void setup()
{
  Serial.begin(115200);
  // inizializzazione delle GPIO
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  sched.addTask(taskSendMsg);
  sched.addTask(taskSemaphore);
  sched.addTask(taskCheckConnections);
  sched.addTask(taskConnection);
  init_mesh();

  // GESTIONE DELLA CONNESSIONE
  nodeList.push_back(IDSEM);
  Serial.println("CONNECTING - SENDING ON_CONNECTION COMMANDS");
  taskConnection.enable();
  taskCheckConnections.enable();
}

void loop()
{

  switch (conn_state)
  {

  case WAITING_CONNECT: // aspetta che si connettano tutti e 4;
  {

    if (nodeList.size() == NUM_CONNECTIONS)
    {
      conn_state = INIT;
      Serial.printf("%d NODES CONNECTED: STARTING\n", NUM_CONNECTIONS);
      taskConnection.disable();
    }

    delay(50);
    break;
  }

  case INIT:
    // set dello stato: primo gruppo rosso, secondo gruppo verde
    state = (GROUPSEM == 0) ? RED : GREEN;
    conn_state = OPERATIVE;
    last_state = IDLE;
    next_state = IDLE;
    break;

  case OPERATIVE:
    if (first_op_loop)
    {
      Serial.printf("STARTING OPERATIVE LOOP\n");
      first_op_loop = false;
      taskSemaphore.enable();
      taskCheckConnections.enable();
    }
    break;

  case DISCONNECTED:
    Serial.println("STARTING FLASHING CASE AND LISTENING FOR NEW CONNECTIONS");
    state = FLASHING;
    conn_state = WAITING_CONNECT;
    taskConnection.enable(); // deve nuovamente ricevere le connessioni dai nodi
    break;
  }
  mesh.update();
}

// ################################ COMUNICAZIONE ################################ //

// Manda il messaggio di cambio di stato al semaforo partner
void sendCommandMsg(command cmd, int to, bool is_red)
{
  StaticJsonDocument<JSONSIZE> jsonDoc;
  jsonMsg.clear();
  jsonDoc["command"] = cmd;
  jsonDoc["last_state"] = last_state;
  jsonDoc["next_state"] = next_state;
  jsonDoc["sender"] = IDSEM;
  jsonDoc["to"] = to;
  if (cmd == IS_RED_CMD)
    jsonDoc["is_red"] = is_red; // questo campo esiste solo se il comando è IS_RED_CMD => risparmio di spazio
  serializeJson(jsonDoc, jsonMsg);
  sendMsg(); // manda il document JSON in broadcast
}

void handle_semaphore()
{

  switch (state)
  {

  case GREEN:
    digitalWrite(RED_LED, LOW); // da verde a rosso => bisogna spegnere
    Serial.println("STARTING GREEN CASE");
    digitalWrite(GREEN_LED, HIGH);
    taskSemaphore.delay(GREEN_TIME); // 2 secondi di delay
    last_state = state;
    next_state = YELLOW;
    state = SWITCH_STATE; // bisogna andare cambiare lo stato
    Serial.println("ENDING GREEN CASE");
    break;

  case YELLOW:
    Serial.println("STARTING YELLOW CASE");
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(YELLOW_LED, HIGH);
    taskSemaphore.delay(YELLOW_TIME);
    last_state = state;
    next_state = RED;
    state = SWITCH_STATE;
    Serial.println("ENDING YELLOW CASE");
    break;

  case RED:
    Serial.println("STARTING RED CASE");
    digitalWrite(YELLOW_LED, LOW);
    digitalWrite(RED_LED, HIGH);
    taskSemaphore.delay(RED_TIME);
    last_state = state;
    next_state = GREEN;
    state = SWITCH_STATE;
    Serial.println("ENDING RED CASE");
    break;

  case SWITCH_STATE:
  {
    Serial.println("STARTING SWITCH_STATE CASE");
    if (GROUPMASTER && last_state != RED)
    { // TODO CASO LAST_STATE == RED ???
      Serial.println("GROUPMASTER: SENDING CHANGE_STATE COMMAND");
      // invio al semaforo partner di cambiare stato
      sendCommandMsg(CHANGE_STATE_CMD, PARTNERSEM);
    }
    if (GROUPMASTER && last_state == RED)
    {
      Serial.println("GROUPMASTER: SENDING IS_GROUP_RED COMMAND");
      sendCommandMsg(IS_GROUP_RED_CMD, PARTNERSEM);
    }
    taskSemaphore.delay(5000); // aspetta la risposta del partner
    Serial.println("ENDING SWITCH_STATE CASE");
    break;
  }

  case WAITING_RED: // aspettiamo il messaggio IS_RED da parte dell'altro gruppo
  {
    Serial.println("STARTING WAITING_RED CASE");
    taskSemaphore.delay(250);
    Serial.println("ENDING WAITING_RED CASE");
    break;
  }

  // task disconneted
  case FLASHING:
  {
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(YELLOW_LED, !digitalRead(YELLOW_LED));
    taskSemaphore.delay(FLASHING_TIME);
    break;
  }
  }
}