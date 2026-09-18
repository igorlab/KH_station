/*
 * =============================================================================
 *  KH station - службовий завантажувач  /  service loader
 * =============================================================================
 *
 * -----------------------------------------------------------------------------
 *  НАЛАШТУЙТЕ ПЕРЕД ЗАЛИВКОЮ
 *  SET UP BEFORE YOU UPLOAD
 * -----------------------------------------------------------------------------
 *
 *  1) В Arduino IDE - три пункти, усі обовʼязкові:
 *     In the Arduino IDE - three settings, all required:
 *
 *    Tools > Board ................. ESP32 Dev Module
 *
 *    Tools > Partition Scheme ...... "Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)"
 *
 *        НЕ дефолтна. Прошивка ~1.6 МБ, а дефолтна схема дає лише 1.31 МБ на
 *        слот - завантаження пройшло б, а встановлення впало б у самому кінці.
 *        NOT the default. The firmware is ~1.6 MB and the default scheme gives
 *        only 1.31 MB per slot: the download would succeed, the install would
 *        fail at the very end.
 *
 *    Tools > Erase All Flash Before Sketch Upload ..... Disabled
 *
 *        !!! ЯКЩО ТУТ Enabled - ЗАЛИВКА СОТРЕ NVS, тобто саме те калібрування,
 *        заради збереження якого цей скетч і існує. Усе доведеться міряти знову.
 *        !!! IF THIS IS Enabled THE UPLOAD WIPES NVS - exactly the calibration
 *        this tool exists to preserve. Everything would have to be measured again.
 *
 *  2) У коді нижче, у блоці ЗАПОВНІТЬ / EDIT, впишіть:
 *     In the code below, in the ЗАПОВНІТЬ / EDIT block, fill in:
 *
 *        WIFI_SSID / WIFI_PASS - назва й пароль вашої Wi-Fi мережі.
 *        Можна лишити порожніми: тоді скетч візьме мережу, до якої станція
 *        вже підключалася сама.
 *        WIFI_SSID / WIFI_PASS - your Wi-Fi network's name and password.
 *        May be left empty: the sketch then uses the network the station
 *        itself was already connecting to.
 *
 *        FW_BIN_URL вже вказує на офіційну прошивку з цього репозиторію -
 *        міняйте лише якщо у вас власний форк.
 *        FW_BIN_URL already points at this repo's official firmware - change
 *        it only if you are running your own fork.
 *
 *  3) Натисніть Upload, відкрийте Serial Monitor на 115200, щоб бачити, що
 *     відбувається.
 *     Press Upload, open Serial Monitor at 115200 to watch it work.
 *
 * -----------------------------------------------------------------------------
 *  ЩО ЦЕ РОБИТЬ
 *  WHAT THIS DOES
 * -----------------------------------------------------------------------------
 *
 *    1. Читає з памʼяті плати (NVS) усі константи калібрування і друкує їх,
 *       щоб у вас був запис ДО того, як щось відбудеться.
 *    2. Переносить старі назви ключів на ті, які читає поточна прошивка.
 *    3. Підключається до Wi-Fi і встановлює прошивку через інтернет.
 *
 *    Калібрування зберігається. Помпи й pH-електрод перекалібровувати не треба.
 *
 *    1. Reads every calibration constant out of the board's NVS and prints it,
 *       so you have a record before anything happens.
 *    2. Migrates the old key names to the ones the current firmware reads.
 *    3. Connects to Wi-Fi and installs the current firmware over the air.
 *    Your calibration survives. No need to calibrate the pumps or probe again.
 *
 * -----------------------------------------------------------------------------
 *  ГОЛОВНЕ: ЩО ЦЕЙ СКЕТЧ НІКОЛИ НЕ РОБИТЬ
 *  THE IMPORTANT PART: WHAT THIS SKETCH NEVER DOES
 * -----------------------------------------------------------------------------
 *
 *    ВІН НІКОЛИ НЕ ПЕРЕЗАПИСУЄ ЗНАЧЕННЯ, ЯКЕ ВЖЕ Є.
 *    IT NEVER OVERWRITES A VALUE THAT ALREADY EXISTS.
 *
 *    Запис у памʼять відбувається РІВНО В ОДНОМУ ВИПАДКУ: коли старий ключ є,
 *    а нового ще немає. Тоді значення копіюється зі старого імені на нове.
 *    A write happens in EXACTLY ONE CASE: the old key exists and the new one
 *    does not. Then the value is copied from the old name to the new one.
 *
 *    Якщо обидва імені вже присутні - значення ЗВІРЯЮТЬСЯ і показуються вам.
 *    Не чіпається нічого, навіть якщо вони різні. Живим вважається НОВИЙ ключ,
 *    бо саме його читає прошивка; старий - це залишок минулої версії.
 *    If both names are present the values are COMPARED and shown to you.
 *    Nothing is touched, even when they differ. The NEW key is the live one -
 *    it is what the firmware reads; the old one is residue.
 *
 *    Скетч НЕ стирає NVS, НЕ форматує флеш, НЕ скидає налаштування.
 *    It does NOT erase NVS, does NOT format flash, does NOT reset anything.
 *
 *    Хочете спершу лише подивитися - поставте DRY_RUN 1 нижче. Тоді не буде
 *    записано жодного байта, а звірка й дамп відпрацюють повністю.
 *    To look first without writing anything, set DRY_RUN to 1 below. The dump
 *    and the comparison still run in full; not a single byte is written.
 * =============================================================================
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <Preferences.h>
#include <esp_ota_ops.h>
#include <esp_wifi.h>

// ======================= ЗАПОВНІТЬ / EDIT ===================================

// Порожньо = мережа, збережена на платі (та, до якої підключалася станція).
// Empty = the network saved on the board (the one the station was using).
static const char* WIFI_SSID = "";      // ЗАПОВНІТЬ: назва вашої Wi-Fi мережі
static const char* WIFI_PASS = "";      // ЗАПОВНІТЬ: пароль від неї

// Офіційна прошивка з цього репозиторію - те саме джерело, що й автооновлення
// самої прошивки (include/OTA.h, URL_fw_Bin). Міняйте лише для власного форку.
// This repo's official firmware - the same source the firmware's own OTA updater
// uses (include/OTA.h, URL_fw_Bin). Change only if you run your own fork.
#define FW_BIN_URL "https://raw.githubusercontent.com/igorlab/KH_station/master/firmware/firmware.bin"

// ========================== ОПЦІЇ / OPTIONS =================================

// 1 = тільки подивитися: дамп і звірка відпрацюють, але В NVS НЕ БУДЕ ЗАПИСАНО
//     НІЧОГО, і прошивка не встановлюватиметься. Безпечно запускати будь-коли.
// 1 = look only: dump and comparison run, NOTHING IS WRITTEN to NVS and no
//     firmware is installed. Safe to run at any time.
#define DRY_RUN 0

// Показувати токен Telegram і ключ labaqua повністю, а не замаскованими.
// Show the Telegram token and labaqua key in full instead of masked.
#define SHOW_SECRETS 0

// Лишіть 0. Ставте 1 тільки якщо скетч не компілюється через відсутність
// набору кореневих сертифікатів - це вимикає перевірку сервера, з якого
// завантажується прошивка.
// Leave at 0. Set to 1 only if the sketch will not compile because your core
// has no certificate bundle - it disables verification of the firmware server.
#define USE_INSECURE_TLS 0

// ============================================================================

#if !USE_INSECURE_TLS
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
// core 3.x wants the length too, and treats size 0 as "do not verify".
extern const uint8_t rootca_crt_bundle_end[] asm("_binary_x509_crt_bundle_end");
#endif

// Прошивка тримає все у цьому просторі імен Preferences. Не змінюйте.
// The firmware keeps everything in this Preferences namespace. Do not change.
static const char* NVS_NAMESPACE = "Settings";

static Preferences prefs;

// Лічильник РЕАЛЬНИХ записів у NVS. Друкується в кінці.
// Counter of ACTUAL writes to NVS. Printed at the end.
static int  nvsWrites  = 0;
static int  mismatches = 0;
static int  untouched  = 0;

// ---------------------------------------------------------- таблиця ключів
// Preferences не вміє перелічувати ключі, тому всі вони перелічені тут явно.
// Тип має збігатися з тим, чим писала прошивка: getFloat() на int32 не працює.
// (putInt і putLong обидва кладуть int32, тому I32 покриває обидва.)

enum KType { K_STR, K_I32, K_U32, K_FLT, K_BOOL, K_BYTES };

struct KeyDef {
  const char* key;
  KType       type;
  bool        secret;
  const char* what;      // українською - це читає оператор
};

static const KeyDef KEYS[] = {
  // --- калібрування помп і електрода: найдорожче, міряне руками ---
  { "cal_r_step",     K_I32,  false, "помпа реагенту, кроків на r_volume" },
  { "cal_w_step",     K_I32,  false, "помпа води, кроків на w_volume" },
  { "cal_e_sec",      K_I32,  false, "помпа зливу, секунд на w_volume" },
  { "r_volume",       K_FLT,  false, "обʼєм шприца реагенту, мл" },
  { "w_volume",       K_FLT,  false, "аліквота води, мл" },
  { "pH1",            K_FLT,  false, "точка калібрування pH 1" },
  { "pH1_adc",        K_FLT,  false, "  її показ АЦП" },
  { "pH2",            K_FLT,  false, "точка калібрування pH 2" },
  { "pH2_adc",        K_FLT,  false, "  її показ АЦП" },
  { "valv_r_f",       K_I32,  false, "клапан реагенту, кут набору" },
  { "valv_r_i",       K_I32,  false, "клапан реагенту, кут подачі" },
  { "valv_w_f",       K_I32,  false, "клапан води, кут набору" },
  { "valv_w_i",       K_I32,  false, "клапан води, кут подачі" },

  // --- налаштування процесу ---
  { "KH_index",       K_FLT,  false, "коефіцієнт корекції KH" },
  { "maxDeviation",   K_FLT,  false, "макс. відхилення KH до перетитрування" },
  { "retitrHour",     K_I32,  false, "титрувати кожні N годин" },
  { "stirrerd",       K_I32,  false, "потужність мішалки (ШІМ)" },
  { "r_Allvolume",    K_FLT,  false, "запас реагенту, літрів" },

  // --- швидкості кроків ---
  { "step_hz",        K_I32,  false, "швидкість кроків, Гц (стара єдина)" },
  { "home_hz",        K_I32,  false, "швидкість хомінгу, Гц" },
  { "fill_hz",        K_I32,  false, "швидкість набору, Гц" },
  { "dose_hz",        K_I32,  false, "швидкість дозування, Гц" },

  // --- фільтр pH ---
  { "ph_mea",         K_FLT,  false, "фільтр pH, похибка вимірювання" },
  { "ph_est",         K_FLT,  false, "фільтр pH, похибка оцінки" },
  { "ph_q",           K_FLT,  false, "фільтр pH, q" },
  { "phSimMode",      K_BOOL, false, "симулятор pH увімкнено" },

  // --- поточний стан ---
  { "lastkh",         K_FLT,  false, "останній прийнятий KH" },
  { "r_usd_vol",      K_FLT,  false, "витрачено реагенту, мл" },
  { "countTitr",      K_U32,  false, "титрувань, усього" },
  { "countTitrReset", K_U32,  false, "титрувань, скидний лічильник" },
  { "version",        K_STR,  false, "версія прошивки, що стартувала востаннє" },

  // --- ідентичність ---
  { "CHAT_ID",        K_STR,  false, "Telegram chat id" },
  { "Es_BOTtoken",    K_STR,  true,  "токен Telegram-бота" },
  { "u_key",          K_STR,  true,  "ключ labaqua" },
  { "dosserIP",       K_STR,  false, "IP дозатора" },
  { "macAddr",        K_BYTES,false, "MAC для ESP-NOW" },

  // --- застарілі, лишені щоб дамп був повним ---
  { "vol_w_ml",       K_FLT,  false, "СТАРЕ: аліквота води" },
  { "vol_r_ml",       K_FLT,  false, "СТАРЕ: обʼєм реагенту" },
  { "state.lastkh",   K_FLT,  false, "СТАРЕ: останній KH" },
  { "state.r_usd_vol",K_FLT,  false, "СТАРЕ: витрачено реагенту" },
  { "state.countTitr",K_U32,  false, "СТАРЕ: лічильник титрувань" },
  { "ssid1",          K_STR,  false, "СТАРЕ: назва Wi-Fi" },
  { "pswrd1",         K_STR,  true,  "СТАРЕ: пароль Wi-Fi" },
};
static const size_t KEY_COUNT = sizeof(KEYS) / sizeof(KEYS[0]);

// ------------------------------------------------------- карта перенесення
// Стара назва -> назва, яку читає поточна прошивка.
//
// Копіюється ТІЛЬКИ тоді, коли нового ключа НЕМАЄ. Якщо є обидва - значення
// звіряються і друкуються, не змінюється нічого.

struct Migration {
  const char* oldKey;
  const char* newKey;
  KType       type;
};

static const Migration MIGRATIONS[] = {
  { "vol_w_ml",        "w_volume",  K_FLT },
  { "vol_r_ml",        "r_volume",  K_FLT },
  { "state.lastkh",    "lastkh",    K_FLT },
  { "state.r_usd_vol", "r_usd_vol", K_FLT },
  { "state.countTitr", "countTitr", K_U32 },
};
static const size_t MIGRATION_COUNT = sizeof(MIGRATIONS) / sizeof(MIGRATIONS[0]);

// ------------------------------------------------------------------ помічники

static String maskSecret(const String& s) {
#if SHOW_SECRETS
  return s;
#else
  if (s.length() <= 8) return F("********");
  return s.substring(0, 4) + "..." + s.substring(s.length() - 4) +
         "  (" + String(s.length()) + " симв.)";
#endif
}

static String readAsText(const KeyDef& k) {
  if (!prefs.isKey(k.key)) return String("-");
  switch (k.type) {
    case K_STR: {
      String v = prefs.getString(k.key, "");
      if (v.length() == 0) return F("\"\"  (порожньо)");
      return k.secret ? maskSecret(v) : ("\"" + v + "\"");
    }
    case K_I32:  return String(prefs.getInt(k.key, 0));
    case K_U32:  return String(prefs.getULong(k.key, 0));
    case K_FLT:  return String(prefs.getFloat(k.key, 0.0f), 4);
    case K_BOOL: return prefs.getBool(k.key, false) ? F("так") : F("ні");
    case K_BYTES: {
      size_t n = prefs.getBytesLength(k.key);
      if (n == 0) return F("-");
      uint8_t buf[16];
      if (n > sizeof(buf)) n = sizeof(buf);
      prefs.getBytes(k.key, buf, n);
      String out;
      for (size_t i = 0; i < n; i++) {
        if (buf[i] < 16) out += '0';
        out += String(buf[i], HEX);
        if (i + 1 < n) out += ':';
      }
      return out;
    }
  }
  return F("?");
}

static void dumpAll() {
  Serial.println();
  Serial.println(F("============ ЩО ЗАРАЗ ЗБЕРЕЖЕНО НА ЦІЙ ПЛАТІ ================"));
  Serial.println(F("            WHAT IS STORED ON THIS BOARD"));
  Serial.println();
  Serial.println(F("  Збережіть цей текст. Це ваше калібрування."));
  Serial.println(F("  Save this text. It is your calibration."));
  Serial.println();
  Serial.printf("  %-16s %-24s %s\n", "КЛЮЧ", "ЗНАЧЕННЯ", "ЩО ЦЕ");
  Serial.println(F("  ------------------------------------------------------------"));

  int present = 0;
  for (size_t i = 0; i < KEY_COUNT; i++) {
    if (prefs.isKey(KEYS[i].key)) present++;
    Serial.printf("  %-16s %-24s %s\n",
                  KEYS[i].key, readAsText(KEYS[i]).c_str(), KEYS[i].what);
  }
  Serial.println(F("  ------------------------------------------------------------"));
  Serial.printf("  Присутньо %d ключів із %d відомих.\n", present, (int)KEY_COUNT);
  Serial.printf("  %d of %d known keys are present.\n", present, (int)KEY_COUNT);

  if (present == 0) {
    Serial.println();
    Serial.println(F("  ПАМʼЯТЬ ПОРОЖНЯ. Це або нова плата, або флеш був стертий."));
    Serial.println(F("  Прошивка стартує з вбудованими значеннями, і помпи та"));
    Serial.println(F("  pH-електрод доведеться калібрувати."));
    Serial.println(F("  NOTHING IS STORED - new board, or the flash was erased."));
  }
}

static void migrateOne(const Migration& m) {
  const bool hasOld = prefs.isKey(m.oldKey);
  const bool hasNew = prefs.isKey(m.newKey);

  // --- обох немає: робити нічого ---
  if (!hasOld && !hasNew) {
    Serial.printf("  %-16s -> %-14s  немає обох, пропуск / neither present\n",
                  m.oldKey, m.newKey);
    return;
  }

  // --- є тільки новий: плата вже на новій прошивці ---
  if (!hasOld && hasNew) {
    untouched++;
    Serial.printf("  %-16s -> %-14s  вже перенесено, НЕ ЧІПАЮ / already done\n",
                  m.oldKey, m.newKey);
    return;
  }

  // --- є тільки старий: ЄДИНИЙ випадок, коли відбувається запис ---
  if (hasOld && !hasNew) {
    String val;
    switch (m.type) {
      case K_FLT: val = String(prefs.getFloat(m.oldKey, 0.0f), 4); break;
      case K_U32: val = String((unsigned long)prefs.getULong(m.oldKey, 0)); break;
      case K_I32: val = String((long)prefs.getInt(m.oldKey, 0)); break;
      default:
        Serial.printf("  %-16s -> %-14s  тип не підтримано, пропуск\n", m.oldKey, m.newKey);
        return;
    }

#if DRY_RUN
    Serial.printf("  %-16s -> %-14s  [DRY_RUN] записав би %s\n",
                  m.oldKey, m.newKey, val.c_str());
    Serial.printf("  %-16s     %-14s  [DRY_RUN] would write %s\n", "", "", val.c_str());
    return;
#else
    switch (m.type) {
      case K_FLT: prefs.putFloat(m.newKey, prefs.getFloat(m.oldKey, 0.0f)); break;
      case K_U32: prefs.putULong(m.newKey, prefs.getULong(m.oldKey, 0));    break;
      case K_I32: prefs.putInt  (m.newKey, prefs.getInt  (m.oldKey, 0));    break;
      default: return;
    }
    nvsWrites++;
    Serial.printf("  %-16s -> %-14s  ПЕРЕНЕСЕНО %s  (новий ключ був порожній)\n",
                  m.oldKey, m.newKey, val.c_str());
    Serial.printf("  %-16s     %-14s  COPIED %s (new key was empty)\n", "", "", val.c_str());
    return;
#endif
  }

  // --- є обидва: ЗВІРЯЄМО, НЕ ПИШЕМО ---
  bool   same = false;
  String oldTxt, newTxt;
  switch (m.type) {
    case K_FLT: {
      const float a = prefs.getFloat(m.oldKey, 0.0f);
      const float b = prefs.getFloat(m.newKey, 0.0f);
      same = fabsf(a - b) < 0.0001f;
      oldTxt = String(a, 4); newTxt = String(b, 4);
      break;
    }
    case K_U32: {
      const uint32_t a = prefs.getULong(m.oldKey, 0);
      const uint32_t b = prefs.getULong(m.newKey, 0);
      same = (a == b);
      oldTxt = String((unsigned long)a); newTxt = String((unsigned long)b);
      break;
    }
    case K_I32: {
      const int32_t a = prefs.getInt(m.oldKey, 0);
      const int32_t b = prefs.getInt(m.newKey, 0);
      same = (a == b);
      oldTxt = String((long)a); newTxt = String((long)b);
      break;
    }
    default:
      Serial.printf("  %-16s -> %-14s  тип не підтримано, пропуск\n", m.oldKey, m.newKey);
      return;
  }

  untouched++;

  if (same) {
    Serial.printf("  %-16s -> %-14s  є обидва, ЗБІГАЮТЬСЯ (%s), НЕ ЧІПАЮ\n",
                  m.oldKey, m.newKey, newTxt.c_str());
    Serial.printf("  %-16s     %-14s  both present, identical, untouched\n", "", "");
  } else {
    mismatches++;
    Serial.println();
    Serial.printf("  %-16s -> %-14s  ** ЗНАЧЕННЯ РІЗНІ / VALUES DIFFER **\n",
                  m.oldKey, m.newKey);
    Serial.printf("      старий  %-14s = %s\n", m.oldKey, oldTxt.c_str());
    Serial.printf("      новий   %-14s = %s   <-- прошивка читає САМЕ ЦЕЙ\n",
                  m.newKey, newTxt.c_str());
    Serial.println(F("      НІЧОГО НЕ ЗМІНЕНО. Новий ключ - живий, старий - залишок"));
    Serial.println(F("      попередньої версії. Рішення за вами, скетч не втручається."));
    Serial.println(F("      NOTHING CHANGED. The new key is live, the old is residue."));
    Serial.println();
  }
}

static void migrateAll() {
  Serial.println();
  Serial.println(F("================= ПЕРЕНЕСЕННЯ КЛЮЧІВ ========================"));
  Serial.println(F("                 KEY MIGRATION"));
  Serial.println();
  Serial.println(F("  Запис відбувається ЛИШЕ там, де старий ключ є, а нового немає."));
  Serial.println(F("  Скрізь, де нове значення вже існує, воно лишається як є."));
  Serial.println(F("  A write happens ONLY where the old key exists and the new"));
  Serial.println(F("  one does not. Existing new values are always left alone."));
#if DRY_RUN
  Serial.println();
  Serial.println(F("  >>> DRY_RUN = 1 : НЕ БУДЕ ЗАПИСАНО ЖОДНОГО БАЙТА <<<"));
  Serial.println(F("  >>> DRY_RUN = 1 : NOT A SINGLE BYTE WILL BE WRITTEN <<<"));
#endif
  Serial.println();

  for (size_t i = 0; i < MIGRATION_COUNT; i++) migrateOne(MIGRATIONS[i]);

  Serial.println(F("  ------------------------------------------------------------"));
  Serial.printf("  ЗАПИСАНО В ПАМʼЯТЬ: %d значень\n", nvsWrites);
  Serial.printf("  WRITTEN TO NVS:     %d values\n", nvsWrites);
  Serial.printf("  Залишено недоторканими: %d   Розбіжностей: %d\n", untouched, mismatches);
  Serial.printf("  Left untouched: %d   Differences found: %d\n", untouched, mismatches);

  if (nvsWrites == 0) {
    Serial.println(F("  Памʼять не змінювалася взагалі."));
    Serial.println(F("  Nothing in memory was modified at all."));
  }
  if (mismatches > 0) {
    Serial.println();
    Serial.println(F("  УВАГА: знайдено розбіжності (вище). Нічого не перезаписано -"));
    Serial.println(F("  прошивка працюватиме зі значеннями під НОВИМИ ключами."));
    Serial.println(F("  NOTE: differences found above. Nothing was overwritten; the"));
    Serial.println(F("  firmware will use the values under the NEW keys."));
  }
}

// Перевіряє розмітку флешу ДО завантаження: помилка тут інакше вилазить аж
// після півтора мегабайта качання.
static bool checkPartitions() {
  Serial.println();
  Serial.println(F("=================== РОЗМІТКА ФЛЕШУ =========================="));

  const esp_partition_t* running = esp_ota_get_running_partition();
  const esp_partition_t* target  = esp_ota_get_next_update_partition(NULL);

  if (running) Serial.printf("  зараз працює з : %-8s  %u байт\n", running->label, running->size);
  if (!target) {
    Serial.println(F("  НЕМАЄ РОЗДІЛУ ДЛЯ ОНОВЛЕННЯ."));
    Serial.println(F("  Обрана схема розділів не має другого слоту, тому оновлення"));
    Serial.println(F("  через інтернет неможливе. В Arduino IDE поставте"));
    Serial.println(F("  Tools > Partition Scheme > Minimal SPIFFS (1.9MB APP with OTA)"));
    Serial.println(F("  і залийте цей скетч ще раз. Калібрування не постраждає."));
    Serial.println(F("  NO OTA TARGET PARTITION - change the Partition Scheme."));
    return false;
  }

  Serial.printf("  встановить у  : %-8s  %u байт\n", target->label, target->size);

  const uint32_t NEEDED = 1500000;
  if (target->size < NEEDED) {
    Serial.println();
    Serial.printf("  ЗАМАЛО. Потрібно щонайменше %u байт, тут %u.\n", NEEDED, target->size);
    Serial.println(F("  У вас дефолтна схема розділів. В Arduino IDE поставте"));
    Serial.println(F("  Tools > Partition Scheme > Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)"));
    Serial.println(F("  і залийте цей скетч ще раз."));
    Serial.println(F("  ВАЖЛИВО: калібрування від цього не постраждає, зміна схеми"));
    Serial.println(F("  розділів не чіпає NVS. Просто перезалийте з правильним пунктом."));
    Serial.println(F("  TOO SMALL - wrong partition scheme. Calibration is NOT affected."));
    return false;
  }

  Serial.println(F("  Схема розділів правильна. / Partition scheme is correct."));
  return true;
}

static bool connectWiFi() {
  Serial.println();
  Serial.println(F("======================== WI-FI ==============================="));

  WiFi.mode(WIFI_STA);

  if (strlen(WIFI_SSID) == 0) {
    // The station saves its network (WiFiManager) in the Wi-Fi driver's own
    // flash area, which uploading this sketch does not touch - reuse it.
    wifi_config_t saved = {};
    esp_wifi_get_config(WIFI_IF_STA, &saved);
    if (saved.sta.ssid[0] == 0) {
      Serial.println(F("  Назву мережі не задано, і на платі немає збереженої мережі."));
      Serial.println(F("  Відкрийте цей скетч, знайдіть рядки з поміткою ЗАПОВНІТЬ"));
      Serial.println(F("  угорі, впишіть назву мережі й пароль, і залийте ще раз."));
      Serial.println(F("  No Wi-Fi name set and none saved on the board - fill in the"));
      Serial.println(F("  lines marked EDIT at the top."));
      return false;
    }
    Serial.printf("  підключаюся до збереженої мережі \"%s\" ", reinterpret_cast<const char*>(saved.sta.ssid));
    WiFi.begin();
  } else {
    Serial.printf("  підключаюся до \"%s\" ", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
  }

  const uint32_t started = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - started < 30000) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("  НЕ ВДАЛОСЯ підключитися за 30 с."));
    Serial.println(F("  Перевірте назву й пароль. ESP32 працює тільки з 2.4 ГГц -"));
    Serial.println(F("  до мережі, яка роздає лише 5 ГГц, він не підключиться ніколи."));
    Serial.println(F("  На платі нічого не змінено, можна просто спробувати ще раз."));
    Serial.println(F("  COULD NOT CONNECT. Nothing on the board was changed."));
    return false;
  }

  Serial.printf("  підключено, IP %s, сигнал %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
  return true;
}

static void onProgress(size_t done, size_t total) {
  static int lastPct = -1;
  const int pct = total ? static_cast<int>(done * 100 / total) : 0;
  if (pct != lastPct && pct % 5 == 0) {
    Serial.printf("  %3d%%  %u / %u байт\n", pct,
                  static_cast<unsigned>(done), static_cast<unsigned>(total));
    lastPct = pct;
  }
}

static void printInstallFailed(const char* what) {
  Serial.println();
  Serial.printf("  НЕ ВДАЛОСЯ: %s\n", what);
  Serial.println();
  Serial.println(F("  ВАШЕ КАЛІБРУВАННЯ НЕ ПОСТРАЖДАЛО, плата працює як раніше."));
  Serial.println(F("  Your calibration is untouched and the board still works."));
  Serial.println(F("  Найчастіші причини:"));
  Serial.println(F("    - не та схема розділів (див. примітку вгорі файлу)"));
  Serial.println(F("    - адреса неправильна або файл недоступний"));
  Serial.println(F("    - мережа обірвалася під час завантаження, спробуйте ще раз"));
}

static void installFirmware() {
  Serial.println();
  Serial.println(F("==================== ВСТАНОВЛЕННЯ ==========================="));

  if (strlen(FW_BIN_URL) == 0) {
    Serial.println(F("  Адресу прошивки не задано. Відкрийте скетч, знайдіть"));
    Serial.println(F("  FW_BIN_URL угорі та впишіть адресу."));
    Serial.println(F("  No firmware address set - fill in FW_BIN_URL at the top."));
    return;
  }

  Serial.printf("  джерело: %s\n", FW_BIN_URL);
  Serial.println(F("  Це займе одну-дві хвилини. НЕ ВИМИКАЙТЕ плату."));
  Serial.println(F("  This takes a minute or two. Do not unplug the board."));
  Serial.println();

  WiFiClientSecure client;
#if USE_INSECURE_TLS
  client.setInsecure();
  Serial.println(F("  УВАГА: сертифікат сервера НЕ перевіряється."));
#else
  client.setCACertBundle(rootca_crt_bundle_start,
                         (size_t)(rootca_crt_bundle_end - rootca_crt_bundle_start));
#endif

  // Not httpUpdate.update(). Arduino core 3's HTTPUpdate checks the first body
  // byte with tcp->peek() != 0xE9, and peek() returns -1 when that byte has not
  // arrived yet - which over TLS it often has not. A perfectly good image is then
  // rejected as "Verify Bin Header Failed" (-106). The firmware hit exactly this
  // (2.4.0/2.4.1, fixed in its include/OTA.h); this sketch had the same call.
  // Update::writeStream() waits for the data instead, and Update itself checks
  // the 0xE9 magic byte on the first block, so no safety is lost.
  HTTPClient http;
  http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
  http.setTimeout(20000);
  if (!http.begin(client, FW_BIN_URL)) {
    printInstallFailed("не вдалося відкрити адресу / could not open the URL");
    return;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    char what[96];
    snprintf(what, sizeof(what), "HTTP %d (%s)", code, http.errorToString(code).c_str());
    http.end();
    printInstallFailed(what);
    return;
  }

  const int len = http.getSize();
  if (len <= 0 || !Update.begin(static_cast<size_t>(len))) {
    http.end();
    printInstallFailed(len <= 0 ? "сервер не повідомив розмір / no size from server"
                                : Update.errorString());
    return;
  }

  Update.onProgress(onProgress);
  const size_t written = Update.writeStream(http.getStream());
  const bool ok = written == static_cast<size_t>(len) && Update.end() && Update.isFinished();
  http.end();

  if (!ok) {
    char what[96];
    snprintf(what, sizeof(what), "записано %u з %d байт: %s",
             static_cast<unsigned>(written), len, Update.errorString());
    Update.abort();
    printInstallFailed(what);
    return;
  }

  Serial.println(F("  Готово. Перезавантаження у нову прошивку."));
  Serial.println(F("  Done. Rebooting into the new firmware."));
  delay(500);
  ESP.restart();
}

void setup() {
  Serial.begin(115200);
  delay(1500);                  // дати монітору підключитися до першого друку

  Serial.println();
  Serial.println(F("============================================================="));
  Serial.println(F("  KH station - службовий завантажувач / service loader"));
  Serial.println(F("============================================================="));
  Serial.printf("  чип %s rev %d, флеш %u МБ\n",
                ESP.getChipModel(), ESP.getChipRevision(),
                ESP.getFlashChipSize() / (1024U * 1024U));
  Serial.printf("  MAC %s\n", WiFi.macAddress().c_str());
#if DRY_RUN
  Serial.println();
  Serial.println(F("  РЕЖИМ ПЕРЕГЛЯДУ (DRY_RUN = 1)"));
  Serial.println(F("  Нічого не буде записано і нічого не буде встановлено."));
  Serial.println(F("  LOOK-ONLY MODE: nothing will be written or installed."));
#endif

  if (!prefs.begin(NVS_NAMESPACE, false)) {
    Serial.println();
    Serial.printf("  Не вдалося відкрити памʼять \"%s\".\n", NVS_NAMESPACE);
    Serial.println(F("  Нічого не змінено. Зупиняюся, щоб не встановити прошивку,"));
    Serial.println(F("  яка потім піднялася б без калібрування."));
    Serial.println(F("  Could not open storage. Nothing changed, stopping."));
    return;
  }

  dumpAll();
  migrateAll();

#if DRY_RUN
  Serial.println();
  Serial.println(F("  DRY_RUN: зупиняюся тут. У памʼять не записано нічого,"));
  Serial.println(F("  прошивка не встановлювалася."));
  Serial.println(F("  Якщо все вище виглядає правильно - поставте DRY_RUN 0"));
  Serial.println(F("  і залийте скетч ще раз."));
  Serial.println(F("  DRY_RUN: stopping here. Set DRY_RUN to 0 and upload again."));
  prefs.end();
  return;
#endif

  if (!checkPartitions()) {
    Serial.println();
    Serial.println(F("  Зупинено перед встановленням. Памʼять не постраждала."));
    Serial.println(F("  Stopped before installing. Memory is intact."));
    prefs.end();
    return;
  }

  if (!connectWiFi()) {
    Serial.println();
    Serial.println(F("  Зупинено перед встановленням. Памʼять не постраждала."));
    Serial.println(F("  Stopped before installing. Memory is intact."));
    prefs.end();
    return;
  }

  // Закрити NVS перед тим, як віддати флеш оновлювачу.
  prefs.end();

  installFirmware();

  Serial.println();
  Serial.println(F("  Завантажувач завершив роботу. Якщо встановлення не вдалося -"));
  Serial.println(F("  усуньте причину вище і натисніть кнопку reset на платі."));
}

void loop() {
  delay(1000);
}
