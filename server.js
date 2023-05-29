const mongoose = require('mongoose');
const WebSocket = require('ws');
const mqtt = require('mqtt');


const dbUrl = 'mongodb+srv://cgomez33:jPwtjOdeGJE5vLMl@cluster0.uooqksx.mongodb.net/Lucecitas';
const wss = new WebSocket.Server({port:8080});        
        

mongoose.connect(dbUrl, { useNewUrlParser: true, useUnifiedTopology: true })
  .then(() => {
    console.log('Conexión exitosa a la base de datos');
  
    const dbName = 'Lucecitas'; 
    const collectionName = 'wiwi'; 

    
    const Schema = mongoose.Schema;
    const DatosSchema = new Schema({
      pir: Boolean,
      humo: Number,
      rfid: Boolean,
      cardiaco: Number
    })

    // Utiliza dbName y collectionName para definir el modelo y la colección
    const DatosModel = mongoose.model(collectionName, DatosSchema);

    wss.on('connection', (ws) => {
        console.log('Cliente conectado');
          
        DatosModel.find().sort().limit(1).then((datos) => {
          ws.send(JSON.stringify(datos[0]));
        });

        const client = mqtt.connect('mqtt://192.168.0.34:1883');
        // Crea un cliente MQTT y conecta al broker
        client.on('connect', () => {
          console.log('Conectado al broker MQTT')
          client.subscribe('/se/luces')
        })

        client.on('message', (topic, message) => {
            let mensaje = JSON.parse(message);
            console.log(`Mensaje recibido en el topic ${topic}: ${JSON.parse(message)}`)

            let documento = new DatosModel({
              pir: mensaje.pir,
              humo: mensaje.humo,
              rfid: mensaje.rfid,
              cardiaco: mensaje.cardiaco
            })

            setInterval(()=>{
              document.save();
            }, 10000);

            ws.send(JSON.stringify(mensaje));
          })
        
        
       
  
        //   client.publish('/se/luces', 'Hola desde Node.js');
  
        // Maneja los mensajes recibidos desde el cliente WebSocket
        ws.on('message', (message) => {
          console.log('Mensaje recibido desde el cliente:', message);
          
          // Publica el mensaje en el topic MQTT
          client.publish('/se/web', message);
        });

        
  
        // Cierra la conexión MQTT y WebSocket cuando el cliente se desconecta
        ws.on('close', () => {
          console.log('Cliente desconectado');
          client.end();
        });
      });
  
      console.log('Servidor web con websockets iniciado en el puerto 8080');
    })
    .catch((error) => {
      console.error('Error al conectar a la base de datos:', error);
    });