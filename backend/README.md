# Varal IoT Backend (FastAPI + AWS IoT)

Backend em FastAPI para receber os heartbeats do ESP32 (via AWS IoT Core)
e expor uma API HTTP para consumo por apps/web, além de enviar comandos
para o dispositivo.

## Estrutura

- `app/main.py` – criação da aplicação FastAPI
- `app/core/config.py` – configurações e carregamento do .env
- `app/core/mqtt_client.py` – cliente MQTT (AWS IoT)
- `app/models/heartbeat.py` – modelo Pydantic do heartbeat
- `app/api/routes/heartbeat.py` – rota GET /heartbeat
- `app/api/routes/commands.py` – rota POST /cmd

## Setup rápido

1. Criar e ativar venv (Windows / PowerShell):

```powershell
python -m venv .venv
.\.venv\Scripts\Activate.ps1
```

2. Instalar dependências:

```powershell
pip install -r requirements.txt
```

3. Copiar `.env.example` para `.env` e ajustar valores (endpoint, caminhos de certs etc.).

4. Colocar os certificados do AWS IoT em `certs/` com os nomes definidos no `.env`.

5. Rodar o servidor:

```powershell
uvicorn app.main:app --reload --port 8000
```

6. Abrir documentação interativa:

- http://localhost:8000/docs
