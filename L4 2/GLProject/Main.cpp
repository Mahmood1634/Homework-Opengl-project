// ============================================================================
//  Homework 2026 - Card game scene (Inscryption / Gwent style table)
//
//  Controls
//    1  -> camera 1: straight above the table, cards lie flat
//    2  -> camera 2: player's point of view, cards stand up
//    G  -> a random card flies from the DECK zone to the PLAY zone
//          (the 6th press does nothing; works in both camera states)
//    RMB+Mouse -> subtle look offset while holding right mouse button
//    ESC-> quit
// ============================================================================
#include "SharedModule.hpp"
#include "Drawer.hpp"
#include "OpenGlShape.hpp"
#include "Camera.hpp"
#include "PolygonWithTexture.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <random>
#include <vector>

// ----------------------------------------------------------------------------
//  Scene constants (world units, Y is up, the player sits on the +Z side)
// ----------------------------------------------------------------------------
namespace Scene
{
  // Cards
  constexpr float kCardW = 1.0f;
  constexpr float kCardH = 1.5f;
  constexpr int kCardCount = 5;
  constexpr float kSlotSpacing = 1.3f;
  constexpr float kStandAngleDeg = 70.0f; // how far a card is tilted up when it "stands"

  // Table
  constexpr float kTableW = 8.4f;         // X size
  constexpr float kTableD = 6.4f;         // Z size
  constexpr float kTableTop = 1.0f;       // Y of the table surface
  constexpr float kZoneOpponentZ = -2.1f; // centre Z of each of the three zones
  constexpr float kZonePlayZ = 0.0f;
  constexpr float kZoneDeckZ = 2.1f;
  constexpr float kZoneDepth = 1.95f;
  constexpr float kCardLift = 0.012f; // cards float a hair above the mats (no z-fighting)

  // Room
  constexpr float kRoomHalfW = 8.0f; // X from -8 to 8
  constexpr float kRoomHalfD = 7.0f; // Z from -7 to 7
  constexpr float kRoomH = 8.0f;

  // Animation timing (seconds)
  constexpr float kCameraTime = 0.9f;
  constexpr float kTiltTime = 0.7f;
  constexpr float kFlyTime = 0.9f;
  constexpr float kTiltStagger = 0.07f;
}

// ----------------------------------------------------------------------------
//  Small helpers
// ----------------------------------------------------------------------------
static float smooth(float t)
{
  t = glm::clamp(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

static float moveToward(float value, float target, float maxDelta)
{
  if (std::fabs(target - value) <= maxDelta)
    return target;
  return value + (target > value ? maxDelta : -maxDelta);
}

static float slotX(int i)
{
  return (i - (Scene::kCardCount - 1) * 0.5f) * Scene::kSlotSpacing;
}

static glm::vec3 pivotFor(int slot, float zoneCentreZ)
{
  return glm::vec3(slotX(slot), Scene::kTableTop + Scene::kCardLift, zoneCentreZ + Scene::kCardH * 0.5f);
}

// ----------------------------------------------------------------------------
//  Cards
// ----------------------------------------------------------------------------
enum class CardState
{
  InDeck,
  Flying,
  Played
};

struct Card
{
  GLuint texture = 0;
  CardState state = CardState::InDeck;
  glm::vec3 pivot{0};     // current position of the near-edge pivot
  glm::vec3 from{0};      // flight start
  glm::vec3 to{0};        // flight end
  float flyT = 0.0f;      // 0..1 flight progress
  float tilt = 0.0f;      // 0 = lying flat, 1 = standing
  float tiltDelay = 0.0f; // stagger so the cards stand up one after another
};

// ----------------------------------------------------------------------------
//  Camera poses
// ----------------------------------------------------------------------------
static const CameraPose kTopCamera = {{0.0f, 8.4f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
static const CameraPose kPlayerCamera = {{0.0f, 4.7f, 6.9f}, {0.0f, 1.8f, -2.0f}, {0.0f, 1.0f, 0.0f}};

// ----------------------------------------------------------------------------
//  Application state (kept in one struct so the GLFW callbacks can reach it)
// ----------------------------------------------------------------------------
struct App
{
  Camera camera{kTopCamera, kPlayerCamera, Scene::kCameraTime};
  float time = 0.0f;

  std::vector<Card> cards;
  int playedCount = 0;
  std::mt19937 rng{std::random_device{}()};

  int framebufferW = 1280, framebufferH = 720;

  void setCamera(int target)
  {
    if (target == camera.getTargetPose())
      return;
    camera.setTargetPose(target);
    for (size_t i = 0; i < cards.size(); ++i)
      cards[i].tiltDelay = Scene::kTiltStagger * static_cast<float>(i);
  }

  void playRandomCard()
  {
    if (playedCount >= Scene::kCardCount)
      return; // 6th press (and later): nothing happens

    std::vector<int> inDeck;
    for (int i = 0; i < static_cast<int>(cards.size()); ++i)
      if (cards[i].state == CardState::InDeck)
        inDeck.push_back(i);
    if (inDeck.empty())
      return;

    const int pick = inDeck[std::uniform_int_distribution<int>(0, static_cast<int>(inDeck.size()) - 1)(rng)];
    Card &c = cards[pick];
    c.state = CardState::Flying;
    c.from = c.pivot;
    c.to = pivotFor(playedCount, Scene::kZonePlayZ);
    c.flyT = 0.0f;
    ++playedCount;
  }

  void onKey(GLFWwindow *window, int key)
  {
    if (key == GLFW_KEY_1 || key == GLFW_KEY_2)
    {
      const int prevTarget = camera.getTargetPose();
      camera.processInput(window, key);
      if (camera.getTargetPose() != prevTarget)
      {
        for (size_t i = 0; i < cards.size(); ++i)
          cards[i].tiltDelay = Scene::kTiltStagger * static_cast<float>(i);
      }
    }
    else if (key == GLFW_KEY_G)
    {
      playRandomCard();
    }
  }

  void update(float dt)
  {
    time += dt;
    camera.update(dt);

    const float targetTilt = static_cast<float>(camera.getTargetPose());
    for (Card &c : cards)
    {
      if (c.tiltDelay > 0.0f)
        c.tiltDelay -= dt;
      else
        c.tilt = moveToward(c.tilt, targetTilt, dt / Scene::kTiltTime);

      if (c.state == CardState::Flying)
      {
        c.flyT += dt / Scene::kFlyTime;
        const float s = smooth(c.flyT);
        c.pivot = glm::mix(c.from, c.to, s);
        c.pivot.y += 1.2f * std::sin(s * glm::pi<float>()); // arc over the table
        if (c.flyT >= 1.0f)
        {
          c.pivot = c.to;
          c.state = CardState::Played;
        }
      }
    }
  }
  float forwardOffset = 0.0f; // (+) للأمام، (-) للخلف
  float moveSpeed = 5.0f;     // سرعة الحركة بالوحدات/ثانية
};

static App *g_app = nullptr;

static void keyCallback(GLFWwindow *window, int key, int, int action, int)
{
  if (action != GLFW_PRESS)
    return;
  if (key == GLFW_KEY_ESCAPE)
  {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
    return;
  }
  if (g_app)
    g_app->onKey(window, key);
}

static void mouseCallback(GLFWwindow *window, double xposIn, double yposIn)
{
  if (g_app)
    g_app->camera.mouse_callback(window, xposIn, yposIn);
}

static void framebufferSizeCallback(GLFWwindow *window, int w, int h)
{
  if (g_app)
  {
    g_app->framebufferW = std::max(w, 1);
    g_app->framebufferH = std::max(h, 1);
    g_app->camera.framebuffer_size_callback(window, g_app->framebufferW, g_app->framebufferH);
  }
  else
  {
    glViewport(0, 0, std::max(w, 1), std::max(h, 1));
  }
}

// ----------------------------------------------------------------------------
//  Scene resources
// ----------------------------------------------------------------------------
struct Shapes
{
  OpenGlShape quadXZ = OpenGlShape::QuadXZ();
  OpenGlShape quadXY = OpenGlShape::QuadXY();
  OpenGlShape box = OpenGlShape::Box();
  OpenGlShape cylinder = OpenGlShape::Cylinder(32);
  OpenGlShape sphere = OpenGlShape::Sphere(32, 48);
};

struct Textures
{
  std::array<GLuint, Scene::kCardCount> card{};
  GLuint cardBack = 0;
  GLuint wall = 0, floor = 0, tableWood = 0, felt = 0, slot = 0, poster = 0;
  GLuint clockFace = 0, darkWood = 0, brass = 0, globe = 0;
};

static Textures loadTextures()
{
  Textures t;
  for (int i = 0; i < Scene::kCardCount; ++i)
    t.card[i] = SharedModule::loadTexture("textures/card_" + std::to_string(i + 1) + ".png", false);
  t.wall = SharedModule::loadTexture("textures/wall.jpg");
  t.floor = SharedModule::loadTexture("textures/floor.jpg");
  t.tableWood = SharedModule::loadTexture("textures/table.jpg");
  t.felt = SharedModule::loadTexture("textures/felt.jpg");
  t.slot = SharedModule::loadTexture("textures/slot.png", false);
  t.poster = SharedModule::loadTexture("textures/poster.jpg", false);
  t.clockFace = SharedModule::loadTexture("textures/clock_face.jpg", false);
  t.darkWood = SharedModule::loadTexture("textures/dark_wood.jpg");
  t.brass = SharedModule::loadTexture("textures/brass.jpg");
  t.globe = SharedModule::loadTexture("textures/globe.jpg");
  t.cardBack = SharedModule::loadTexture("textures/backG.png", false);
  return t;
}

// ----------------------------------------------------------------------------
//  Drawing helpers
// ----------------------------------------------------------------------------
static glm::mat4 T(const glm::vec3 &v) { return glm::translate(glm::mat4(1.0f), v); }
static glm::mat4 S(const glm::vec3 &v) { return glm::scale(glm::mat4(1.0f), v); }
static glm::mat4 R(float rad, const glm::vec3 &axis) { return glm::rotate(glm::mat4(1.0f), rad, axis); }

static Material mat(GLuint tex, glm::vec4 tint = glm::vec4(1.0f), glm::vec2 uv = glm::vec2(1.0f),
                    float spec = 0.15f, float shine = 32.0f, ShadeMode mode = ShadeMode::Lit)
{
  Material m;
  m.texture = tex;
  m.tint = tint;
  m.uvScale = uv;
  m.specular = spec;
  m.shininess = shine;
  m.mode = mode;
  return m;
}

static void drawRoom(const Drawer &d, const Shapes &s, const Textures &t)
{
  using namespace Scene;
  const float W = kRoomHalfW * 2.0f, D = kRoomHalfD * 2.0f;

  d.draw(s.quadXZ, S({W, 1.0f, D}), mat(t.floor, glm::vec4(1.0f), {8.0f, 7.0f}, 0.25f, 48.0f));

  const Material wall = mat(t.wall, glm::vec4(1.0f), {8.0f, 4.0f}, 0.02f, 8.0f);
  d.draw(s.quadXY, T({0, kRoomH / 2, -kRoomHalfD}) * S({W, kRoomH, 1}), wall);
  d.draw(s.quadXY, T({0, kRoomH / 2, kRoomHalfD}) * R(glm::pi<float>(), {0, 1, 0}) * S({W, kRoomH, 1}), wall);
  d.draw(s.quadXY, T({-kRoomHalfW, kRoomH / 2, 0}) * R(glm::half_pi<float>(), {0, 1, 0}) * S({D, kRoomH, 1}),
         mat(t.wall, glm::vec4(1.0f), {7.0f, 4.0f}, 0.02f, 8.0f));
  d.draw(s.quadXY, T({kRoomHalfW, kRoomH / 2, 0}) * R(-glm::half_pi<float>(), {0, 1, 0}) * S({D, kRoomH, 1}),
         mat(t.wall, glm::vec4(1.0f), {7.0f, 4.0f}, 0.02f, 8.0f));

  d.draw(s.box, T({0, 0.18f, -kRoomHalfD + 0.04f}) * S({W, 0.36f, 0.08f}), mat(t.darkWood, glm::vec4(1.0f), {12.0f, 1.0f}));

  d.draw(s.quadXY, T({0.8f, 4.3f, -kRoomHalfD + 0.02f}) * S({3.0f, 4.0f, 1.0f}),
         mat(t.poster, glm::vec4(1.0f), {1, 1}, 0.0f, 8.0f, ShadeMode::Poster));
}

static void drawTable(const Drawer &d, const Shapes &s, const Textures &t)
{
  using namespace Scene;
  const Material wood = mat(t.felt, glm::vec4(1.0f), {3.0f, 2.0f}, 0.30f, 40.0f);

  d.draw(s.box, T({0, kTableTop - 0.125f, 0}) * S({kTableW, 0.25f, kTableD}), wood);
  const float lx = kTableW / 2 - 0.35f, lz = kTableD / 2 - 0.35f;
  for (int sx = -1; sx <= 1; sx += 2)
    for (int sz = -1; sz <= 1; sz += 2)
      d.draw(s.box, T({sx * lx, (kTableTop - 0.25f) / 2, sz * lz}) * S({0.45f, kTableTop - 0.25f, 0.45f}), wood);

  const float matW = 7.4f;
  struct Zone
  {
    float z;
    glm::vec4 tint;
  };
  const Zone zones[3] = {
      {kZoneOpponentZ, {0.16f, 0.16f, 0.18f, 1.0f}}, // أسود فحمي / جرافيت غامق
      {kZonePlayZ, {0.34f, 0.36f, 0.39f, 1.0f}},     // رمادي فولاذي متوسط (واضح ومحايد)
      {kZoneDeckZ, {0.58f, 0.54f, 0.47f, 1.0f}},     // عاجي / رمادي فاتح دافئ (بارز ومحايد)
  };
  for (const Zone &z : zones)
    d.draw(s.quadXZ, T({0, kTableTop + 0.003f, z.z}) * S({matW, 1.0f, kZoneDepth}),
           mat(t.felt, z.tint, {4.0f, 1.0f}, 0.0f, 8.0f));

  for (const Zone &z : zones)
    for (int i = 0; i < kCardCount; ++i)
      d.draw(s.quadXZ, T({slotX(i), kTableTop + 0.007f, z.z}) * S({kCardW + 0.12f, 1.0f, kCardH + 0.12f}),
             mat(t.slot, glm::vec4(1.0f), {1, 1}, 0.0f, 8.0f, ShadeMode::Unlit));
}

static void drawCards(const Drawer &d, const Shapes &s, const Textures &t, const App &app)
{
  using namespace Scene;
  for (size_t i = 0; i < app.cards.size(); ++i)
  {
    const Card &c = app.cards[i];
    const float angle = glm::radians(kStandAngleDeg) * smooth(c.tilt);

    float roll = 0.0f;
    if (c.state == CardState::Flying)
      roll = 0.35f * std::sin(smooth(c.flyT) * glm::pi<float>());

    const glm::mat4 model = T(c.pivot) * R(roll, {0, 0, 1}) * R(angle, {1, 0, 0}) *
                            T({0, 0, -kCardH * 0.5f}) * S({kCardW, 1.0f, kCardH});

    PolygonWithTexture cardPoly(s.quadXZ, t.card[i]);
    cardPoly.setMaterialParams(0.25f, 48.0f, ShadeMode::Card);
    cardPoly.transformation(model);
    cardPoly.draw(d);
  }
}
static void drawOpponentCards(const Drawer &d, const Shapes &s, const Textures &t)
{
  using namespace Scene;
  for (int i = 0; i < kCardCount; ++i)
  {
    const glm::vec3 oppPivot(slotX(i), kTableTop + kCardLift, kZoneOpponentZ - kCardH * 0.5f);
    const float oppTilt = glm::radians(-20.0f);

    const glm::mat4 model = T(oppPivot) * R(glm::pi<float>(), {0, 1, 0}) * R(oppTilt, {1, 0, 0}) *
                            T({0, 0, -kCardH * 0.5f}) * S({kCardW, 1.0f, kCardH});

    PolygonWithTexture oppCard(s.quadXZ, t.cardBack, glm::vec4(1.0f));
    oppCard.setMaterialParams(0.20f, 40.0f, ShadeMode::FaceDown);
    oppCard.transformation(model);
    oppCard.draw(d);
  }
}

static void drawGrandfatherClock(const Drawer &d, const Shapes &s, const Textures &t, float time)
{
  const glm::mat4 base = T({-6.2f, 0.0f, -6.2f});
  const Material wood = mat(t.felt, glm::vec4(1.0f), {1, 2}, 0.35f, 40.0f);
  const Material brass = mat(t.brass, glm::vec4(1.0f), {1, 1}, 0.8f, 64.0f);

  d.draw(s.box, base * T({0, 0.25f, 0}) * S({1.5f, 0.5f, 1.0f}), wood);
  d.draw(s.box, base * T({0, 2.2f, 0}) * S({1.1f, 3.4f, 0.8f}), wood);
  d.draw(s.box, base * T({0, 4.45f, 0}) * S({1.5f, 1.3f, 0.95f}), wood);
  d.draw(s.box, base * T({0, 5.2f, 0}) * S({1.7f, 0.2f, 1.1f}), wood);
  d.draw(s.quadXY, base * T({0, 4.45f, 0.48f}) * S({1.1f, 1.1f, 1.0f}), mat(t.clockFace, glm::vec4(1.0f), {1, 1}, 0.2f, 32.0f));
  d.draw(s.box, base * T({0, 2.35f, 0.41f}) * S({0.62f, 2.2f, 0.04f}), mat(t.darkWood, glm::vec4(0.18f, 0.2f, 0.22f, 1.0f)));

  const float swing = 0.30f * std::sin(time * 2.4f);
  const glm::mat4 pend = base * T({0, 3.35f, 0.43f}) * R(swing, {0, 0, 1});
  d.draw(s.cylinder, pend * T({0, -1.7f, 0}) * S({0.025f, 1.7f, 0.025f}), brass);
  d.draw(s.cylinder, pend * T({0, -1.85f, 0}) * R(glm::half_pi<float>(), {1, 0, 0}) * S({0.22f, 0.04f, 0.22f}) * T({0, -0.5f, 0}), brass);
}

static void drawGlobe(const Drawer &d, const Shapes &s, const Textures &t, float time)
{
  const glm::mat4 base = T({6.3f, 0.0f, -5.6f});
  const Material wood = mat(t.darkWood, glm::vec4(1.0f), {1, 1}, 0.3f, 32.0f);
  const Material brass = mat(t.brass, glm::vec4(1.0f), {1, 1}, 0.8f, 64.0f);

  d.draw(s.cylinder, base * S({0.55f, 0.12f, 0.55f}), wood);
  d.draw(s.cylinder, base * T({0, 0.12f, 0}) * S({0.07f, 1.35f, 0.07f}), brass);
  d.draw(s.cylinder, base * T({0, 1.47f, 0}) * S({0.18f, 0.06f, 0.18f}), brass);
  const glm::mat4 g = base * T({0, 2.25f, 0}) * R(glm::radians(23.5f), {0, 0, 1}) * R(time * 0.35f, {0, 1, 0});
  d.draw(s.sphere, g * S({0.75f, 0.75f, 0.75f}), mat(t.globe, glm::vec4(1.0f), {1, 1}, 0.35f, 48.0f));
  d.draw(s.cylinder, g * T({0, -0.9f, 0}) * S({0.02f, 1.8f, 0.02f}), brass);
}

static void drawTelescope(const Drawer &d, const Shapes &s, const Textures &t)
{
  const glm::mat4 base = T({3.9f, 0.0f, -5.4f}) * R(glm::radians(-25.0f), {0, 1, 0});
  const Material wood = mat(t.darkWood, glm::vec4(1.0f), {1, 3}, 0.3f, 32.0f);
  const Material brass = mat(t.brass, glm::vec4(1.0f), {1, 1}, 0.85f, 72.0f);

  for (int i = 0; i < 3; ++i)
  {
    const float yaw = glm::radians(120.0f * i);
    d.draw(s.cylinder, base * T({0, 1.75f, 0}) * R(yaw, {0, 1, 0}) * R(glm::radians(180.0f - 16.0f), {1, 0, 0}) * S({0.045f, 1.8f, 0.045f}), wood);
  }
  d.draw(s.cylinder, base * T({0, 1.72f, 0}) * S({0.16f, 0.12f, 0.16f}), brass);

  const glm::mat4 tube = base * T({0, 1.9f, 0}) * R(glm::radians(-58.0f), {1, 0, 0});
  d.draw(s.cylinder, tube * T({0, -0.5f, 0}) * S({0.11f, 1.9f, 0.11f}), brass);
  d.draw(s.cylinder, tube * T({0, 1.15f, 0}) * S({0.17f, 0.45f, 0.17f}), brass);
  d.draw(s.cylinder, tube * T({0, -0.75f, 0}) * S({0.06f, 0.25f, 0.06f}), mat(t.darkWood));
}

#ifdef SELFTEST
static void saveScreenshot(const char *name, int w, int h)
{
  std::vector<unsigned char> px(static_cast<size_t>(w) * h * 3);
  glPixelStorei(GL_PACK_ALIGNMENT, 1);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, px.data());
  FILE *f = std::fopen(name, "wb");
  if (!f)
    return;
  std::fprintf(f, "P6\n%d %d\n255\n", w, h);
  for (int y = h - 1; y >= 0; --y)
    std::fwrite(&px[static_cast<size_t>(y) * w * 3], 1, static_cast<size_t>(w) * 3, f);
  std::fclose(f);
}
#endif

static void run(GLFWwindow *window)
{
  App app;
  g_app = &app;
  glfwSetKeyCallback(window, keyCallback);
  glfwSetCursorPosCallback(window, mouseCallback);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwGetFramebufferSize(window, &app.framebufferW, &app.framebufferH);
  glViewport(0, 0, app.framebufferW, app.framebufferH);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_MULTISAMPLE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  Drawer drawer(SharedModule::createShaderProgram("shaders/vs/L4.vs", "shaders/fs/L4.fs"));
  Shapes shapes;
  Textures textures = loadTextures();

  app.cards.resize(Scene::kCardCount);
  for (int i = 0; i < Scene::kCardCount; ++i)
  {
    app.cards[i].texture = textures.card[i];
    app.cards[i].pivot = pivotFor(i, Scene::kZoneDeckZ);
  }

  double lastTime = glfwGetTime();
#ifdef SELFTEST
  int frame = 0;
#endif

  while (!glfwWindowShouldClose(window))
  {
    double now = glfwGetTime();
    float dt = static_cast<float>(now - lastTime);
    lastTime = now;
    dt = std::min(dt, 0.1f);
#ifdef SELFTEST
    dt = 1.0f / 30.0f;
#endif
    app.update(dt);

    glClearColor(0.05f, 0.04f, 0.04f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::vec3 eye;
    const glm::mat4 view = app.camera.getViewMatrix(eye);
    const float aspect = static_cast<float>(app.framebufferW) / static_cast<float>(app.framebufferH);
    const glm::mat4 proj = glm::perspective(glm::radians(55.0f), aspect, 0.1f, 100.0f);

    const float blend = smooth(app.camera.getBlend());
    const float tableLightScale = glm::mix(1.0f, 0.45f, blend);

    drawer.beginFrame(view, proj, eye, app.time, tableLightScale);
    drawRoom(drawer, shapes, textures);
    drawTable(drawer, shapes, textures);
    drawGrandfatherClock(drawer, shapes, textures, app.time);
    drawGlobe(drawer, shapes, textures, app.time);
    drawTelescope(drawer, shapes, textures);
    drawCards(drawer, shapes, textures, app);
#ifdef SELFTEST
    ++frame;
    if (frame == 5)
      saveScreenshot("shot_1_top_start.ppm", app.framebufferW, app.framebufferH);
    if (frame == 10 || frame == 22 || frame == 34)
      app.onKey(window, GLFW_KEY_G);
    if (frame == 40)
      saveScreenshot("shot_2_top_midflight.ppm", app.framebufferW, app.framebufferH);
    if (frame == 90)
      saveScreenshot("shot_3_top_3played.ppm", app.framebufferW, app.framebufferH);
    if (frame == 95)
      app.onKey(window, GLFW_KEY_2);
    if (frame == 105)
      saveScreenshot("shot_4_transition.ppm", app.framebufferW, app.framebufferH);
    if (frame == 170)
      saveScreenshot("shot_5_player_3played.ppm", app.framebufferW, app.framebufferH);
    if (frame == 175 || frame == 176)
      app.onKey(window, GLFW_KEY_G);
    if (frame == 180)
      app.onKey(window, GLFW_KEY_G);
    if (frame == 185)
      std::printf("playedCount after 6 presses = %d (expected 5)\n", app.playedCount);
    if (frame == 260)
      saveScreenshot("shot_6_player_all5.ppm", app.framebufferW, app.framebufferH);
    if (frame == 265)
      app.onKey(window, GLFW_KEY_1);
    if (frame == 360)
    {
      saveScreenshot("shot_7_top_all5.ppm", app.framebufferW, app.framebufferH);
      break;
    }
#endif

    glfwSwapBuffers(window);
    glfwPollEvents();
  }
}

int main()
{
  GLFWwindow *window = SharedModule::createWindow(1280, 720, "Card Game Scene");
  if (!window)
    return -1;

  run(window);

  glfwTerminate();
  return 0;
}