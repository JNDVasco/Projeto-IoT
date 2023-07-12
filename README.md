# Projeto-IoT

Repositório Projeto IoT 

## Cloud Infrastructure

The cloud infrastructure will be running on a private server. Each service will be a container and they will be interconnected by docker networks.

- **EMQX** as the main MQTT Broker

- **InfluxDB** as the main database

- **Telegraf** as the data scraper to save the JSON on the DB

- **Grafana** as the main dashboard

![Exemplo Condução diferencial](Images/service-diagram.png "Condução Diferencial")

## JSON Format

**Aquatic Nodes iot/water**

```json
{
    "node_type": "water", 
    "node_name": "Pacific Tank",
    "data": {
        "temp": 12.54,
        "ph": 7.7
    }
}
```

**Air Nodes iot/air**

```json
# 
{
    "node_type": "air", 
    "node_name": "North Hall",
    "data": {
        "temp": 12.54,
        "humidity": 77,
        "CO": 1.2,
        "movement": "True"
    }
}
```

**Status iot/status**

```json
{
    "node_type": "air", 
    "node_name": "North Hall",
    "data": {
        "batt": 3300,
        "temp": 20.00,
    }
}
```
