#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define STARTING_HP 30
#define MAX_HAND 6
#define STARTING_HAND 3
#define MAX_SCORES 100
#define MAX_NAME 30

typedef struct
{
    char name[20];
    int power;
    char type;
} Card;

Card cardPool[] =
{
    {"Fireball",     15, 'A'},
    {"Slash",         8, 'A'},
    {"Heavy Strike", 12, 'A'},
    {"Quick Jab",     5, 'A'},
    {"Meteor",       20, 'A'},
    {"Heal Potion",  10, 'H'},
    {"Regen",         6, 'H'},
    {"Iron Shield",  10, 'S'},
    {"Guard",         6, 'S'},
    {"Life Drain",    7, 'A'},
};
const int POOL_SIZE = sizeof(cardPool) / sizeof(cardPool[0]);

typedef struct StackNode
{
    Card card;
    struct StackNode *next;
} StackNode;

typedef struct
{
    StackNode *top;
    int size;
} Stack;

void stackInit(Stack *s)
{
    s->top = NULL;
    s->size = 0;
}

void stackPush(Stack *s, Card c)
{
    StackNode *node = (StackNode *)malloc(sizeof(StackNode));
    node->card = c;
    node->next = s->top;
    s->top = node;
    s->size++;
}

int stackIsEmpty(Stack *s)
{
    return s->top == NULL;
}

Card stackPop(Stack *s)
{
    StackNode *old = s->top;
    Card c = old->card;
    s->top = old->next;
    free(old);
    s->size--;
    return c;
}

typedef struct HandNode
{
    Card card;
    struct HandNode *next;
} HandNode;

void handInsert(HandNode **head, Card c)
{
    HandNode *node = (HandNode *)malloc(sizeof(HandNode));
    node->card = c;

    if (*head == NULL || (*head)->card.power < c.power)
    {
        node->next = *head;
        *head = node;
        return;
    }
    HandNode *cur = *head;
    while (cur->next != NULL && cur->next->card.power >= c.power)
        cur = cur->next;
    node->next = cur->next;
    cur->next = node;
}

int handCount(HandNode *head)
{
    int n = 0;
    while (head)
    {
        n++;
        head = head->next;
    }
    return n;
}

Card handRemoveAt(HandNode **head, int index)
{
    HandNode *cur = *head;
    Card removed;
    if (index == 0)
    {
        removed = cur->card;
        *head = cur->next;
        free(cur);
        return removed;
    }
    HandNode *prev = NULL;
    for (int i = 0; i < index; i++)
    {
        prev = cur;
        cur = cur->next;
    }
    removed = cur->card;
    prev->next = cur->next;
    free(cur);
    return removed;
}

void freeHand(HandNode *head)
{
    while (head)
    {
        HandNode *next = head->next;
        free(head);
        head = next;
    }
}

void printHand(HandNode *head)
{
    int i = 0;
    HandNode *cur = head;
    while (cur)
    {
        const char *typeLabel = cur->card.type == 'A' ? "ATK" :
                                cur->card.type == 'H' ? "HEAL" : "SHLD";
        printf("  [%d] %-14s (%s, power %d)\n", i, cur->card.name, typeLabel, cur->card.power);
        i++;
        cur = cur->next;
    }
}

typedef struct QueueNode
{
    int playerId;
    struct QueueNode *next;
} QueueNode;

typedef struct
{
    QueueNode *front, *rear;
} TurnQueue;

void turnQueueInit(TurnQueue *q)
{
    q->front = q->rear = NULL;
}

void turnEnqueue(TurnQueue *q, int id)
{
    QueueNode *node = (QueueNode *)malloc(sizeof(QueueNode));
    node->playerId = id;
    node->next = NULL;
    if (q->rear == NULL)
    {
        q->front = q->rear = node;
        return;
    }
    q->rear->next = node;
    q->rear = node;
}

int turnDequeue(TurnQueue *q)
{
    QueueNode *node = q->front;
    int id = node->playerId;
    q->front = q->front->next;
    if (q->front == NULL) q->rear = NULL;
    free(node);
    return id;
}

typedef struct
{
    char name[MAX_NAME];
    int hp;
    int shield;
    HandNode *hand;
} Fighter;

Stack drawPile, discardPile;

void shuffleStackFromPool(Stack *s, int copies)
{
    int total = POOL_SIZE * copies;
    Card *temp = (Card *)malloc(sizeof(Card) * total);
    for (int c = 0; c < copies; c++)
        for (int i = 0; i < POOL_SIZE; i++)
            temp[c * POOL_SIZE + i] = cardPool[i];

    for (int i = total - 1; i > 0; i--)
    {
        int j = rand() % (i + 1);
        Card t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
    }
    for (int i = 0; i < total; i++) stackPush(s, temp[i]);
    free(temp);
}

void drawCard(Fighter *f)
{
    if (handCount(f->hand) >= MAX_HAND) return;

    if (stackIsEmpty(&drawPile))
    {
        if (stackIsEmpty(&discardPile)) return;
        printf("(Draw pile empty — reshuffling discard pile...)\n");
        while (!stackIsEmpty(&discardPile))
            stackPush(&drawPile, stackPop(&discardPile));

        int n = drawPile.size;
        Card *temp = (Card *)malloc(sizeof(Card) * n);
        for (int i = 0; i < n; i++) temp[i] = stackPop(&drawPile);
        for (int i = n - 1; i > 0; i--)
        {
            int j = rand() % (i + 1);
            Card t = temp[i];
            temp[i] = temp[j];
            temp[j] = t;
        }
        for (int i = 0; i < n; i++) stackPush(&drawPile, temp[i]);
        free(temp);
    }

    Card c = stackPop(&drawPile);
    handInsert(&f->hand, c);
}

void applyCard(Fighter *attacker, Fighter *defender, Card c)
{
    if (c.type == 'A')
    {
        int dmg = c.power;
        if (defender->shield > 0)
        {
            int absorbed = dmg < defender->shield ? dmg : defender->shield;
            defender->shield -= absorbed;
            dmg -= absorbed;
            printf("%s's shield absorbs %d damage!\n", defender->name, absorbed);
        }
        if (dmg > 0)
        {
            defender->hp -= dmg;
            printf("%s plays %s for %d damage!\n", attacker->name, c.name, dmg);
        }
    }
    else if (c.type == 'H')
    {
        attacker->hp += c.power;
        printf("%s plays %s and heals %d HP!\n", attacker->name, c.name, c.power);
    }
    else if (c.type == 'S')
    {
        attacker->shield += c.power;
        printf("%s plays %s and gains %d shield!\n", attacker->name, c.name, c.power);
    }
}

int aiChooseIndex(Fighter *ai)
{
    int idx = 0, i = 0;
    HandNode *cur = ai->hand;
    HandNode *bestHeal = NULL;
    int bestHealIdx = -1;
    HandNode *bestAttack = NULL;
    int bestAttackIdx = -1;

    while (cur)
    {
        if (cur->card.type == 'H' && (!bestHeal || cur->card.power > bestHeal->card.power))
        {
            bestHeal = cur;
            bestHealIdx = i;
        }
        if (cur->card.type == 'A' && (!bestAttack || cur->card.power > bestAttack->card.power))
        {
            bestAttack = cur;
            bestAttackIdx = i;
        }
        cur = cur->next;
        i++;
    }

    if (ai->hp <= 12 && bestHeal) idx = bestHealIdx;
    else if (bestAttack) idx = bestAttackIdx;
    else idx = 0;

    return idx;
}

typedef struct
{
    char name[MAX_NAME];
    int turns;
} ScoreEntry;

ScoreEntry leaderboard[MAX_SCORES];
int scoreCount = 0;
const char *SCORE_FILE = "leaderboard.txt";

void loadLeaderboard(void)
{
    FILE *f = fopen(SCORE_FILE, "r");
    if (!f) return;
    scoreCount = 0;
    while (scoreCount < MAX_SCORES &&
            fscanf(f, "%29s %d", leaderboard[scoreCount].name, &leaderboard[scoreCount].turns) == 2)
    {
        scoreCount++;
    }
    fclose(f);
}

void saveLeaderboard(void)
{
    FILE *f = fopen(SCORE_FILE, "w");
    if (!f) return;
    for (int i = 0; i < scoreCount; i++)
        fprintf(f, "%s %d\n", leaderboard[i].name, leaderboard[i].turns);
    fclose(f);
}

void insertScore(const char *name, int turns)
{
    if (scoreCount >= MAX_SCORES) return;
    ScoreEntry entry;
    strncpy(entry.name, name, MAX_NAME - 1);
    entry.name[MAX_NAME - 1] = '\0';
    entry.turns = turns;

    int i = scoreCount - 1;
    while (i >= 0 && leaderboard[i].turns > entry.turns)
    {
        leaderboard[i + 1] = leaderboard[i];
        i--;
    }
    leaderboard[i + 1] = entry;
    scoreCount++;
}

void trimToTop(int n)
{
    while (scoreCount > n) scoreCount--;
}

void printLeaderboard(void)
{
    printf("\nLEADERBOARD (fewest turns to win)\n");
    if (scoreCount == 0)
    {
        printf("  (empty)\n");
        return;
    }
    for (int i = 0; i < scoreCount; i++)
        printf("  %d. %-15s %d turns\n", i + 1, leaderboard[i].name, leaderboard[i].turns);
}

int main(void)
{
    srand((unsigned int)time(NULL));
    loadLeaderboard();

    Fighter player = {"You", STARTING_HP, 0, NULL};
    Fighter ai = {"Computer", STARTING_HP, 0, NULL};

    stackInit(&drawPile);
    stackInit(&discardPile);
    shuffleStackFromPool(&drawPile, 2);

    for (int i = 0; i < STARTING_HAND; i++)
    {
        drawCard(&player);
        drawCard(&ai);
    }

    TurnQueue turns;
    turnQueueInit(&turns);
    turnEnqueue(&turns, 0);
    turnEnqueue(&turns, 1);

    printf("=== CARD DUEL ===\n");
    printf("Defeat the Computer! Both start at %d HP.\n", STARTING_HP);

    int turnCount = 0;
    int gameOver = 0;
    int playerWon = 0;

    while (!gameOver)
    {
        int who = turnDequeue(&turns);
        Fighter *actor = (who == 0) ? &player : &ai;
        Fighter *other = (who == 0) ? &ai : &player;

        drawCard(actor);

        printf("\nTurn %d: %s's move (HP %d, shield %d | %s HP %d, shield %d)\n",
               turnCount + 1, actor->name, actor->hp, actor->shield,
               other->name, other->hp, other->shield);

        int chosenIdx;
        if (who == 0)
        {
            printf("Your hand:\n");
            printHand(player.hand);
            int handSize = handCount(player.hand);
            do
            {
                printf("Choose a card [0-%d]: ", handSize - 1);
                int result = scanf("%d", &chosenIdx);
                if (result != 1)
                {
                    if (result == EOF)
                    {
                        printf("\nNo more input — exiting.\n");
                        freeHand(player.hand);
                        freeHand(ai.hand);
                        return 0;
                    }
                    chosenIdx = -1;
                    int ch;
                    while ((ch = getchar()) != '\n' && ch != EOF) { }
                    if (ch == EOF)
                    {
                        printf("\nNo more input — exiting.\n");
                        freeHand(player.hand);
                        freeHand(ai.hand);
                        return 0;
                    }
                }
            }
            while (chosenIdx < 0 || chosenIdx >= handSize);
        }
        else
        {
            chosenIdx = aiChooseIndex(&ai);
        }

        Card played = handRemoveAt(&actor->hand, chosenIdx);
        applyCard(actor, other, played);
        stackPush(&discardPile, played);

        if (other->hp <= 0)
        {
            gameOver = 1;
            playerWon = (who == 0);
            turnCount++;
            break;
        }

        turnEnqueue(&turns, who);
        if (who == 1) turnCount++;
    }

    if (playerWon)
    {
        printf("*** YOU WIN in %d turns! ***\n", turnCount);
        char name[MAX_NAME] = "Player";
        printf("Enter your name for the leaderboard: ");
        if (scanf(" %29[^\n]", name) != 1) strcpy(name, "Player");
        insertScore(name, turnCount);
        trimToTop(5);
        saveLeaderboard();
        printLeaderboard();
    }
    else
    {
        printf("You were defeated by the Computer. Try again!\n");
    }

    freeHand(player.hand);
    freeHand(ai.hand);
    while (!stackIsEmpty(&drawPile)) stackPop(&drawPile);
    while (!stackIsEmpty(&discardPile)) stackPop(&discardPile);

    return 0;
}
