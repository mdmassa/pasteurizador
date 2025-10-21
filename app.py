from flask import Flask, render_template, request
import paho.mqtt.client as mqtt
import threading

app = Flask(__name__)

# Estado inicial
status = {
    "temperaturas": {
        "a": "0.0", 
        "b": "0.0", 
        "r": "0.0", 
        "media": "0.0"
    },
    "reles": {
        "ebulidor": False,
        "peltier": False
    }
}

# Configuração MQTT - USE SUAS CREDENCIAIS AQUI
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
MQTT_USER = "massa"          # SEU USUÁRIO
MQTT_PASS = "admin"          # SUA SENHA

# Tópicos (ajuste se necessário)
TOPICO_TEMP_A = "pasteurizador/temperatura/aquecedorA"
TOPICO_TEMP_B = "pasteurizador/temperatura/aquecedorB"
TOPICO_TEMP_R = "pasteurizador/temperatura/refrigerador"
TOPICO_TEMP_MEDIA = "pasteurizador/temperatura/media"
TOPICO_CMD_EBULIDOR = "pasteurizador/comando/ebulidor"
TOPICO_CMD_PELTIER = "pasteurizador/comando/peltier"

# Cliente MQTT
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(MQTT_USER, MQTT_PASS)

def on_connect(client, userdata, flags, rc):
    print("Conectado ao broker MQTT")
    client.subscribe(TOPICO_TEMP_A)
    client.subscribe(TOPICO_TEMP_B)
    client.subscribe(TOPICO_TEMP_R)
    client.subscribe(TOPICO_TEMP_MEDIA)

def on_message(client, userdata, msg):
    try:
        payload = msg.payload.decode()
        topic = msg.topic
        
        if topic == TOPICO_TEMP_A:
            status["temperaturas"]["a"] = payload
        elif topic == TOPICO_TEMP_B:
            status["temperaturas"]["b"] = payload
        elif topic == TOPICO_TEMP_R:
            status["temperaturas"]["r"] = payload
        elif topic == TOPICO_TEMP_MEDIA:
            status["temperaturas"]["media"] = payload
            
        print(f"Atualizado: {topic} = {payload}")
        
    except Exception as e:
        print(f"Erro ao processar mensagem: {str(e)}")

mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# Iniciar thread MQTT
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_thread = threading.Thread(target=mqtt_client.loop_forever)
mqtt_thread.daemon = True
mqtt_thread.start()

@app.route("/")
def index():
    return render_template("index.html", status=status)

@app.route("/controle", methods=["POST"])
def controle():
    dispositivo = request.form["dispositivo"]
    acao = request.form["acao"]
    
    topico = TOPICO_CMD_EBULIDOR if dispositivo == "ebulidor" else TOPICO_CMD_PELTIER
    comando = "LIGAR" if acao == "ligar" else "DESLIGAR"
    
    mqtt_client.publish(topico, comando)
    status["reles"][dispositivo] = (acao == "ligar")
    
    print(f"Comando enviado: {topico} - {comando}")
    return "OK"

if __name__ == "__main__":
    app.run(host='0.0.0.0', port=5000)
