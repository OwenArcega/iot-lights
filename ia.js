const mongoose = require('mongoose');
const brain = require('brain.js');
const DataScheme =(new mongoose.Schema(
    {
        sensors:{
            cardiaco:{
                type: Number
            }
        }
    }, 
     //{ collection: 'wiwis' }
    )
    );
const model = mongoose.model('wiwis', DataScheme);


//marcar el cerebro
let net = new brain.recurrent.LSTMTimeStep();
        
let prediccion;
        
exports.getData= async (req,res)=>{
    const llamadaDatos =  await model.find();
    var query =await  model.find();
    let datosParaCorrerModelo = [];
    let arrayDatos = [];
    //vamos a darle todos los valores al array
    llamadaDatos.forEach(element=>{
        if(element.sensors.cardiaco!=undefined){
            arrayDatos.push(element.sensors.cardiaco);
        }
    })

    query.forEach(element=>{
        if(element.sensors.cardiaco!=undefined){
            datosParaCorrerModelo.push(element.sensors.cardiaco);
        }
    })
    // entrenamiento
    net.train([
        arrayDatos
    ]);
    //prediccion
      let runData = [datosParaCorrerModelo[datosParaCorrerModelo.length-1]]; 
      console.log(runData)
      let result = net.run(runData);
    prediccion = result;
    return prediccion;
}