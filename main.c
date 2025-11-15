#include <math.h>
#include <raylib.h>
#include <time.h>

#define PADDING 30
#define SCREEN_WIDTH 45 * 24 + (PADDING * 4) + (PADDING / 3 * 3)
#define SCREEN_HEIGHT 45 * 6 + (PADDING * 2)
#define ANIM_DURATION 20.0f

typedef struct {
  double hours_hand_angle;
  double minutes_hand_angle;
} AnalogClock;

typedef struct {
  int value;
  AnalogClock analog_clocks[4][6];
} Cell;

typedef struct {
  Cell cells[6];
} DigitalClock;

#define DegToRad(deg) deg *DEG2RAD

double NormalizeAngle(double angle) {
  while (angle < 0)
    angle += 360;
  while (angle >= 360)
    angle -= 360;
  return angle;
}

Cell GetCellForNumber(int num);

void UpdateDigitalClock(DigitalClock *clock) {
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  int hours = timeinfo->tm_hour;
  int minutes = timeinfo->tm_min;
  int seconds = timeinfo->tm_sec;

  int digits[6] = {hours / 10,   hours % 10,   minutes / 10,
                   minutes % 10, seconds / 10, seconds % 10};

  for (int i = 0; i < 6; i++)
    clock->cells[i] = GetCellForNumber(digits[i]);
}

Vector2 PointOnCircle(int centerX, int centerY, double radius, double angle) {
  double rad = DegToRad(NormalizeAngle(angle));
  return (Vector2){.x = centerX + radius * cos(rad),
                   .y = centerY - radius * sin(rad)};
}

static float GetAngleDifference(float from, float to) {
  if (from == to)
    return 0;
  float d = to - from;
  while (d <= 0.0f)
    d += 360.0f;
  while (d > 360.0f)
    d -= 360.0f;
  return d;
}

static float InterpolateHand(float old_angle, float new_angle, int frame) {
  float diff = GetAngleDifference(old_angle, new_angle);

  if (diff < 0.001f)
    return new_angle;

  float step = diff / ANIM_DURATION;
  float moved = step * frame;
  return NormalizeAngle(old_angle + moved);
}

DigitalClock GetClockToDraw(const DigitalClock *old_clock,
                            const DigitalClock *new_clock, int frame) {
  if (frame >= (int)ANIM_DURATION)
    return *new_clock;

  DigitalClock result = {0};

  for (int cellIdx = 0; cellIdx < 6; ++cellIdx) {
    Cell oldCell = old_clock->cells[cellIdx];
    Cell newCell = new_clock->cells[cellIdx];

    Cell interpolated = {.value = newCell.value};

    for (int x = 0; x < 4; ++x) {
      for (int y = 0; y < 6; ++y) {
        AnalogClock *oldAC = &oldCell.analog_clocks[x][y];
        AnalogClock *newAC = &newCell.analog_clocks[x][y];

        interpolated.analog_clocks[x][y].hours_hand_angle = InterpolateHand(
            oldAC->hours_hand_angle, newAC->hours_hand_angle, frame);

        interpolated.analog_clocks[x][y].minutes_hand_angle = InterpolateHand(
            oldAC->minutes_hand_angle, newAC->minutes_hand_angle, frame);
      }
    }
    result.cells[cellIdx] = interpolated;
  }
  return result;
}

void DrawClock(DigitalClock *clock) {
  int analogClockDiameter = (SCREEN_HEIGHT - (2 * PADDING)) / 6;
  int analogClockRedius = analogClockDiameter / 2;

  int cellWidth = analogClockDiameter * 4;
  int cellHeight = analogClockDiameter * 6;

  int cellX = PADDING;
  int cellY = PADDING;

  for (int cellIndex = 0; cellIndex < 6; cellIndex++) {
    Cell cell = clock->cells[cellIndex];

    for (int analogClockXPos = 0; analogClockXPos < 4; analogClockXPos++) {
      for (int analogClockYPos = 0; analogClockYPos < 6; analogClockYPos++) {
        int circleCenterX =
            cellX + analogClockRedius + (analogClockXPos * analogClockDiameter);
        int circleCenterY =
            cellY + analogClockRedius + (analogClockYPos * analogClockDiameter);
        Vector2 circleVec = {.x = circleCenterX, .y = circleCenterY};

        DrawCircleGradient(circleCenterX, circleCenterY, analogClockRedius - 2,
                           GRAY, WHITE);

        AnalogClock analogClock =
            cell.analog_clocks[analogClockXPos][analogClockYPos];

        Vector2 hoursHand =
            PointOnCircle(circleCenterX, circleCenterY, analogClockRedius,
                          analogClock.hours_hand_angle);
        DrawLineEx(circleVec, hoursHand, 2, BLACK);

        Vector2 minsHand =
            PointOnCircle(circleCenterX, circleCenterY, analogClockRedius,
                          analogClock.minutes_hand_angle);
        DrawLineEx(circleVec, minsHand, 2, BLACK);

        DrawCircleLinesV(circleVec, analogClockRedius, WHITE);
      }
    }

    if (cellIndex < 5) {
      float gap = (cellIndex % 2 == 0) ? (PADDING / 3) : PADDING;
      cellX += cellWidth + gap;
    }
  }
}

int main(void) {
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Analog Digital Clock");
  SetTargetFPS(60);

  DigitalClock current_clock = {0};
  DigitalClock old_clock = {0};
  DigitalClock draw_clock = {0};

  UpdateDigitalClock(&current_clock);
  old_clock = current_clock;
  draw_clock = current_clock;

  int last_second = -1;
  int frame_counter = 0;

  while (!WindowShouldClose()) {
    time_t now = time(0);
    struct tm *ti = localtime(&now);
    int cur_sec = ti->tm_sec;

    if (cur_sec != last_second) {
      old_clock = current_clock;
      UpdateDigitalClock(&current_clock);
      frame_counter = 0;
      last_second = cur_sec;
    }

    BeginDrawing();
    ClearBackground(WHITE);
    draw_clock = GetClockToDraw(&old_clock, &current_clock, frame_counter);
    DrawClock(&draw_clock);
    EndDrawing();
    frame_counter++;
  }

  CloseWindow();
  return 0;
}

Cell GetCellForNumber(int num) {
  Cell cell = {.value = num};
  for (int x = 0; x < 4; x++)
    for (int y = 0; y < 6; y++) {
      cell.analog_clocks[x][y].hours_hand_angle = 225;
      cell.analog_clocks[x][y].minutes_hand_angle = 225;
    }

  switch (num) {
  case 0:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][1] = (AnalogClock){0, 270};
    cell.analog_clocks[2][1] = (AnalogClock){180, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[0][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){90, 270};
    cell.analog_clocks[2][2] = (AnalogClock){90, 270};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[0][3] = (AnalogClock){90, 270};
    cell.analog_clocks[1][3] = (AnalogClock){90, 270};
    cell.analog_clocks[2][3] = (AnalogClock){90, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){90, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 90};
    cell.analog_clocks[2][4] = (AnalogClock){90, 180};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 1:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){0, 90};
    cell.analog_clocks[1][1] = (AnalogClock){180, 270};
    cell.analog_clocks[2][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){90, 270};
    cell.analog_clocks[2][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][3] = (AnalogClock){90, 270};
    cell.analog_clocks[2][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){0, 270};
    cell.analog_clocks[1][4] = (AnalogClock){90, 180};
    cell.analog_clocks[2][4] = (AnalogClock){0, 90};
    cell.analog_clocks[3][4] = (AnalogClock){180, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 2:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){0, 90};
    cell.analog_clocks[1][1] = (AnalogClock){0, 180};
    cell.analog_clocks[2][1] = (AnalogClock){180, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[0][2] = (AnalogClock){0, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 180};
    cell.analog_clocks[2][2] = (AnalogClock){90, 180};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[0][3] = (AnalogClock){90, 270};
    cell.analog_clocks[1][3] = (AnalogClock){0, 270};
    cell.analog_clocks[2][3] = (AnalogClock){0, 180};
    cell.analog_clocks[3][3] = (AnalogClock){90, 180};
    cell.analog_clocks[0][4] = (AnalogClock){90, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 90};
    cell.analog_clocks[2][4] = (AnalogClock){0, 180};
    cell.analog_clocks[3][4] = (AnalogClock){180, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 3:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){0, 90};
    cell.analog_clocks[1][1] = (AnalogClock){0, 180};
    cell.analog_clocks[2][1] = (AnalogClock){180, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 270};
    cell.analog_clocks[2][2] = (AnalogClock){90, 180};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][3] = (AnalogClock){0, 90};
    cell.analog_clocks[2][3] = (AnalogClock){180, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){0, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 180};
    cell.analog_clocks[2][4] = (AnalogClock){90, 180};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 4:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){180, 270};
    cell.analog_clocks[2][0] = (AnalogClock){0, 270};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][1] = (AnalogClock){90, 270};
    cell.analog_clocks[2][1] = (AnalogClock){90, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[0][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 90};
    cell.analog_clocks[2][2] = (AnalogClock){90, 180};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[0][3] = (AnalogClock){0, 90};
    cell.analog_clocks[1][3] = (AnalogClock){0, 180};
    cell.analog_clocks[2][3] = (AnalogClock){180, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[2][4] = (AnalogClock){90, 270};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[2][5] = (AnalogClock){0, 90};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 5:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][1] = (AnalogClock){0, 270};
    cell.analog_clocks[2][1] = (AnalogClock){0, 180};
    cell.analog_clocks[3][1] = (AnalogClock){90, 180};
    cell.analog_clocks[0][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 90};
    cell.analog_clocks[2][2] = (AnalogClock){0, 180};
    cell.analog_clocks[3][2] = (AnalogClock){180, 270};
    cell.analog_clocks[0][3] = (AnalogClock){0, 90};
    cell.analog_clocks[1][3] = (AnalogClock){0, 180};
    cell.analog_clocks[2][3] = (AnalogClock){180, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){0, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 180};
    cell.analog_clocks[2][4] = (AnalogClock){90, 180};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 6:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][1] = (AnalogClock){0, 270};
    cell.analog_clocks[2][1] = (AnalogClock){0, 180};
    cell.analog_clocks[3][1] = (AnalogClock){90, 180};
    cell.analog_clocks[0][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 90};
    cell.analog_clocks[2][2] = (AnalogClock){0, 180};
    cell.analog_clocks[3][2] = (AnalogClock){180, 270};
    cell.analog_clocks[0][3] = (AnalogClock){90, 270};
    cell.analog_clocks[1][3] = (AnalogClock){0, 270};
    cell.analog_clocks[2][3] = (AnalogClock){180, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){90, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 90};
    cell.analog_clocks[2][4] = (AnalogClock){90, 180};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 7:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){0, 90};
    cell.analog_clocks[1][1] = (AnalogClock){0, 180};
    cell.analog_clocks[2][1] = (AnalogClock){180, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[2][2] = (AnalogClock){90, 270};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[2][3] = (AnalogClock){90, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[2][4] = (AnalogClock){90, 270};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[2][5] = (AnalogClock){0, 90};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 8:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][1] = (AnalogClock){0, 270};
    cell.analog_clocks[2][1] = (AnalogClock){180, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[0][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 90};
    cell.analog_clocks[2][2] = (AnalogClock){90, 180};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[0][3] = (AnalogClock){90, 270};
    cell.analog_clocks[1][3] = (AnalogClock){0, 270};
    cell.analog_clocks[2][3] = (AnalogClock){180, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){90, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 90};
    cell.analog_clocks[2][4] = (AnalogClock){90, 180};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  case 9:
    cell.analog_clocks[0][0] = (AnalogClock){0, 270};
    cell.analog_clocks[1][0] = (AnalogClock){0, 180};
    cell.analog_clocks[2][0] = (AnalogClock){0, 180};
    cell.analog_clocks[3][0] = (AnalogClock){180, 270};
    cell.analog_clocks[0][1] = (AnalogClock){90, 270};
    cell.analog_clocks[1][1] = (AnalogClock){0, 270};
    cell.analog_clocks[2][1] = (AnalogClock){180, 270};
    cell.analog_clocks[3][1] = (AnalogClock){90, 270};
    cell.analog_clocks[0][2] = (AnalogClock){90, 270};
    cell.analog_clocks[1][2] = (AnalogClock){0, 90};
    cell.analog_clocks[2][2] = (AnalogClock){90, 180};
    cell.analog_clocks[3][2] = (AnalogClock){90, 270};
    cell.analog_clocks[0][3] = (AnalogClock){0, 90};
    cell.analog_clocks[1][3] = (AnalogClock){0, 180};
    cell.analog_clocks[2][3] = (AnalogClock){180, 270};
    cell.analog_clocks[3][3] = (AnalogClock){90, 270};
    cell.analog_clocks[0][4] = (AnalogClock){0, 270};
    cell.analog_clocks[1][4] = (AnalogClock){0, 180};
    cell.analog_clocks[2][4] = (AnalogClock){90, 180};
    cell.analog_clocks[3][4] = (AnalogClock){90, 270};
    cell.analog_clocks[0][5] = (AnalogClock){0, 90};
    cell.analog_clocks[1][5] = (AnalogClock){0, 180};
    cell.analog_clocks[2][5] = (AnalogClock){0, 180};
    cell.analog_clocks[3][5] = (AnalogClock){90, 180};
    break;
  }

  return cell;
}
