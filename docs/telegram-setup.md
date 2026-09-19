# Setting up Telegram

The station talks to you through a Telegram bot of your own. Before you set it up you
need two things from Telegram:

1. a **bot token** — the bot's key, which the station uses to send and receive messages
2. your **Telegram ID** — a number that tells the station who it should listen to

It takes about five minutes, and you only do it once.

---

## 1. Create the bot

1. In Telegram, open [@BotFather](https://t.me/BotFather) — Telegram's own bot for
   making bots (it has a blue check mark) — and press **Start**.
2. Send `/newbot`.
3. BotFather asks for a **name**. That is what shows up in your chat list, for example
   `My KH station`. Anything works, and you can change it later.
4. Then it asks for a **username**: 5–32 characters, Latin letters, digits and `_`, and
   it must end in `bot`, for example `reef_kh_station_bot`. It has to be unique across
   all of Telegram, so if it is taken, try another. It can't be changed later.
5. BotFather replies with a link to your new bot and, below "Use this token to access
   the HTTP API:", the token. It looks like this:

   ```
   1234567890:AAHdqTcvCH1vGWJxfSeofSAs0K5PALDsaw
   ```

   Copy all of it — the digits, the colon and everything after. That is your **bot
   token**.
6. Tap the link to your bot (`t.me/<your bot's username>`) and press **Start**. Don't
   skip this: Telegram doesn't let a bot write to anyone who hasn't written to it first,
   so until you press Start the station can't send you anything.

> **Keep the token to yourself.** Whoever has it can read and send your bot's messages.
> If it ever leaks — a screenshot, a public post — get a new one in BotFather:
> `/mybots` → your bot → **API Token** → **Revoke current token**, then give the new
> token to the station the same way as below. The old one stops working right away.

---

## 2. Find your Telegram ID

Your ID is a number like `123456789`. It is **not** your @username and not your phone
number. There are two ways to get it.

### The quick way

Open [@userinfobot](https://t.me/userinfobot) and press **Start**. It answers with a
few lines about you; the number next to **Id** is what you need.

It is a third-party bot, not part of Telegram. It sees what any bot sees when you
message it — your name, username and ID — and nothing more.

### Without a third-party bot

Your own bot can tell you, as long as the station isn't using it yet:

1. Send your new bot any message, e.g. `hi`. It won't answer — that's fine.
2. Open this address in a browser, with your token in place of `<TOKEN>`:

   ```
   https://api.telegram.org/bot<TOKEN>/getUpdates
   ```

   The word `bot` goes straight in front of the token, with nothing in between:
   `https://api.telegram.org/bot1234567890:AAHdq.../getUpdates`
3. The page is a wall of text. Press **Ctrl+F** (**Cmd+F** on a Mac), search for
   `"chat":{"id":`, and the number right after it is your ID:

   ```
   "chat":{"id":123456789,"first_name":"...","type":"private"}
   ```

If the page shows only `{"ok":true,"result":[]}`, send the bot another message and
reload. Once the station has the token it collects the bot's messages itself every few
seconds, and this page stays empty — use @userinfobot then.

---

## 3. Give both to the station

The first time, this has to be done on the station's setup page or over USB — the
bot doesn't know you yet, so it won't take commands from you in Telegram.

**On the setup page.** Open `http://kh-station.local/settings` (or
`http://<device-ip>/settings`, or the **station setup** link at the bottom of the
dashboard). In the **Telegram** box, paste the token into **Bot token** and your ID
into **Your chat ID**, and press **Save**. The page comes back with "Saved: Telegram bot
token, chat ID", and under each field shows what the station has stored now — the
token only by its last six characters. Empty fields are left as they are, so later you
can change one of them without retyping the other.

(Firmware older than 2.5.15 has a plainer page: the fields are **Telegram Bot token**
and **Telegram UserID**, the button is **Submit**, and it doesn't show what is stored.)

**Or over USB**, in a serial monitor at 115200 baud, one line at a time:

```
bottoken_1234567890:AAHdqTcvCH1vGWJxfSeofSAs0K5PALDsaw
settlgrmid_123456789
```

The station answers `New BOTtoken accepted (...)` with the token's length and last four
characters — it never prints the token back in full — and `Telegram ID = 123456789`.

**Check it works.** No restart is needed. Send your bot `lastkh`; the answer should come
within about ten seconds, with the station's buttons under the message field. From then
on you can do everything from the chat.

---

## If the bot stays silent

- **You didn't press Start in your bot.** Open it and press Start (or send any message).
- **The ID is wrong.** It must be digits only — no @username, no spaces. The station
  obeys exactly one chat, so with a wrong ID it answers nobody, and you can't fix it
  from Telegram. The setup page shows the ID the station has now, under the field —
  compare it with yours and enter it again if it differs.
- **The token is incomplete.** It must be the whole line from BotFather, digits before
  the colon included. The station only refuses one that is clearly too short (it replies
  `BOTtoken is wrong`), so a token that lost a few characters on the way can still be
  accepted. If in doubt, copy it from BotFather again and re-enter it.
- **The station isn't online.** Open `http://kh-station.local/` — if the dashboard
  doesn't load, sort out Wi-Fi first (see the main [README](../README.md)).

If somebody else finds your bot and writes to it, the station ignores them and sends
you a `Spammer: <their name> <their id>` message, so you'll know.

**Moving to another Telegram account?** From your current chat, send
`settlgrmid_<new id>`. From that moment the station listens only to the new account —
remember to press Start in the bot there too.
