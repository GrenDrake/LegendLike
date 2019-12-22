#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_STATS       8
#define NUM_TYPES       16
#define MAX_EVO_LEVEL   50

const char *typeNames[NUM_TYPES] = {
    "None",  "Fire",     "Ice",    "Water",
    "Stone", "Electric", "Wind",   "Ethereal",
    "Mind",  "Flying",   "Metal",  "Poison",
    "Void",  "Light",    "Shadow", "Wood"
};
const char *statNames[NUM_STATS] = {
    "Attack",   "Defense",  "Mind",     "Willpower",
    "Speed",    "Health",   "Energy",   "XP"
};

void generate_stats(int *stat_array, int points) {
    stat_array[7] = points / NUM_STATS;
    int s = rand() % (NUM_STATS - 1);
    while (points > 0) {
        ++stat_array[s];
        --points;
        if (rand() % 100 < 30)  s = rand() % (NUM_STATS - 1);
    }
    int xpmod = (rand() % 11) - 5;
    stat_array[7] += xpmod;
}

void make_beast(FILE *out, int ident, int *t1, int *t2, int *t3, int stat_points_lv0) {
    fprintf(out, "* Beast %d *\nTypes:", ident);

    if (*t1 < 0) {
        *t1 = rand() % NUM_TYPES;
        if (*t1 != 0 && rand() % 100 < 50) {
            *t2 = rand() % NUM_TYPES;
            if (*t2 != 0 && rand() % 100 < 25) {
                *t3 = rand() % NUM_TYPES;
            }
        }
    } else {
        if (*t1 && *t2 == -1 && rand() % 100 < 30) {
            *t2 = rand() % NUM_TYPES;
        } else if (*t2 && *t3 == -1 && rand() % 100 < 30) {
            *t3 = rand() % NUM_TYPES;
        }
    }
    fprintf(out, " %s/%d", typeNames[*t1], *t1);
    if (*t2 > 0) fprintf(out, ", %s/%d", typeNames[*t2], *t2);
    if (*t3 > 0) fprintf(out, ", %s/%d", typeNames[*t3], *t3);

    int stat_array[NUM_STATS] = {0};
    fprintf(out, "\n             Atck Defn Mind Will Sped Hlth Ergy XP\n");
    fprintf(out, "Stats Lv0    ");
    generate_stats(stat_array, stat_points_lv0);
    int total = 0;
    for (int j = 0; j < NUM_STATS; ++j) {
        fprintf(out, "%-4d ", stat_array[j]);
        if (j != NUM_STATS - 1) total += stat_array[j];
    }

    for (int i = 0; i < NUM_STATS; ++i) {
        double m = 10.0;
        double mod = ((rand() % 4000) - 2000) / 1000.0;
        stat_array[i] *= m + mod;
    }

    fprintf(out, " = %d\nStats Lv100  ", total);
    total = 0;
    for (int j = 0; j < NUM_STATS; ++j) {
        fprintf(out, "%-4d ", stat_array[j]);
        if (j != NUM_STATS - 1) total += stat_array[j];
    }
    fprintf(out, " = %d\n\n", total);
}


int main(void) {
    srand(time(0));

    int evoCounts[20] = { 0 };

    int beastNo = 1;
    int toMake = 50;

    FILE *out = fopen("beasts.txt", "wt");
    if (!out) {
        printf("Failed to open output file.\n");
        return 1;
    }

    for (int i = 0; i < toMake; ++i) {
        int t1 = -1, t2 = -1, t3 = -1;
        int pBase = 190, pMod = 20;

        int evolutionCount[] = {
            1, 2, 2, 2,
            2, 3, 3, 4
        };

        beastNo = rand() % 10000;
        int evoCount = evolutionCount[rand() % 8];
        ++evoCounts[evoCount];

        int basePoints = pBase + rand() % pMod;
        fprintf(out, "--=== THE BEAST %d FAMILY ===--\n", beastNo);
        fprintf(out, "%d forms\n\n", evoCount);
        if (evoCount == 1) {
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.5);
        } else if (evoCount == 2) {
            int e1 = 10 + rand() % 30;
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.333);
            fprintf(out, "EVOLUTION AT %d\n\n", e1);
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.666);
        } else if (evoCount == 3) {
            int e1 = 10 + rand() % 30;
            int e2 = e1 + 5 + rand() % 30;
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.25);
            fprintf(out, "EVOLUTION AT %d\n\n", e1);
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.5);
            fprintf(out, "EVOLUTION AT %d\n\n", e2);
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.75);
        } else if (evoCount == 4) {
            int e1 = 10 + rand() % 30;
            int e2 = e1 + 5 + rand() % 30;
            int e3 = e2 + 5 + rand() % 30;
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.2);
            fprintf(out, "EVOLUTION AT %d\n\n", e1);
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.4);
            fprintf(out, "EVOLUTION AT %d\n\n", e2);
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.6);
            fprintf(out, "EVOLUTION AT %d\n\n", e3);
            make_beast(out, beastNo++, &t1, &t2, &t3, basePoints * 1.8);
        } else {
            printf("BAD EVO COUNT: %d\n", evoCount);
        }


        for (int j = 0; j < evoCount; ++j) {
            pMod *= 1.5;
            pBase *= 1.5;
        }
        fprintf(out, "\n\n");
    }

    for (int i = 0; i < 5; ++i) {
        if (evoCounts[i] > 0) {
            fprintf(out, "%d evolutions: %d\n", i, evoCounts[i]);
        }
    }
    fprintf(out, "\n");
    return 0;
}