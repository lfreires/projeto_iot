# Varal IoT – Aplicativo mobile

Aplicativo React Native (Expo) para acompanhar o varal automático conectado (ESP32 + backend FastAPI). Nesta primeira versão exibimos os dados principais (temperatura, umidade, chuva e uptime) recebidos do endpoint `/heartbeat/` do backend.

## Pré-requisitos

- Node.js 18 ou superior
- `npm` 9+ ou `pnpm/yarn` (ajuste os comandos se necessário)
- Backend rodando localmente (por padrão em `http://10.0.2.2:8000`) ou um endpoint público disponível

## Instalação

```powershell
cd front
npm install
```

## Execução

```powershell
npm run start
```

O comando abrirá o CLI do Expo. Escolha `a` para Android, `i` para iOS (no macOS) ou escaneie o QR Code com o aplicativo Expo Go.

## Configurando o endpoint da API

- Ambiente local (Android Emulator): usamos `http://10.0.2.2:8000` como padrão.
- Para trocar o endpoint, defina a variável `EXPO_PUBLIC_API_URL` antes de iniciar o Metro bundler:

```powershell
$env:EXPO_PUBLIC_API_URL="https://seu-backend.com"; npm run start
```

Também é possível editar `app.json` e adicionar em `expo.extra.apiBaseUrl`.

## Estrutura

```
front/
  App.tsx                # ponto de entrada
  src/
    components/          # UI reutilizável (cards, badges, linhas)
    hooks/               # hooks especializados (useHeartbeat)
    screens/             # telas; atualmente apenas Dashboard
    services/            # comunicação com o backend (fetchHeartbeat)
    theme/               # tokens de design (cores, espaçamento, tipografia)
    utils/               # funções de formatação
```

## Próximos passos sugeridos

- Implementar autenticação e envio de comandos (OPEN/CLOSE/AUTO) para o endpoint `/cmd`.
- Adicionar testes de componentes com `@testing-library/react-native`.
- Configurar CI para lint + typecheck automatizados.
