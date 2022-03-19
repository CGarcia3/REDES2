# REDES2
Servidor que responde peticiones GET, POST y OPTIONS.  
Concurrente, hace fork() para cada petición que recibe.

## Uso
Desde la carpeta de practica1 hacemos make para compilar y crear ejecutables.  
Luego ejecutamos el server con ./server.  
Éste escuchará peticiones por el puerto 8080 del localhost.  
Podemos cambiar la configuración mediante el fichero server.conf.