// Optional standalone Telegram bot logic (not required to run project)

const axios = require("axios");

const BOT_TOKEN = process.env.BOT_TOKEN;
const CHAT_ID = process.env.CHAT_ID;

function sendAlert(message) {
  const url = `https://api.telegram.org/bot${BOT_TOKEN}/sendMessage`;
  return axios.get(url, {
    params: { chat_id: CHAT_ID, text: message }
  });
}

module.exports = { sendAlert };
