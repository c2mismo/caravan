# HIGH VOLTAGE

Proyecto para automatizar una caja de electricidad de 230 voltios de una autocaravana con arduino.


## Definicion de la caja

La caja dispone de:

### 3 Entradas de corriente.

Una proviene del inversor el cual es activado solo bajo demanda através del arduino "general_relays", para ello dispone de su driver correspondiente instalado en el inversor.

Las otras dos son similares pero independientes cada una dispuesta hacia el exterior a sendos laterales. Estos pueden tanto adquirir como ofrecer 220V indiferentemente.

### 4 Salidas de corriente.

Las dos tomas exteriores, que como dijimos pueden tanto adquirir como ofrecer 220V indiferentemente.

La alimentación de la vivienda.

El cargador de baterías de 230V. Este solo puede ser activado cuando la alimentación de 220V no provenga del inversor.

## Todas las salidas estan devidamente protegidas:

### De forma pasiva

* Dispone de Protección de armónicos (Filtro EMI).

* Protección Sobretensión Transitoria.

* Protección Sobretensión Permanente.

* Diferencial.

* Magnetotérmico.

### De forma activa (através del arduino)

* Protección de sobretensión y bajotensión de la batería.

* Protección de sobrecorriente.

* Control y monotorización de su actividad y consumo.

* Control y reducción de la temperatura con dos ventiladores extractores.
  Despues de test de temperatura se ha decidido anular los ventiladores.


## Dispone de botones para la realización de maniobras manuales.

Estas solo activan/desactivan el arduino y los relés.

## Los relés

Al disponer de un tamaño muy reducido y necesitar cinco relés, no se han encontrado ninguno bipolar que se ajuste a las medidas, por ello dispone de diez relés de estado sólido que quedan conectados para que actúen de forma paralela (bipolar) por circuito.
Se dispone de otro relé este electromecánico para alimentar el cargador de baterias 220V,
comandado gracias al mosfet IRF520.

## Comunicación

La comunicación se realiza através de un módulo SPI to CANbus.


## Recursos para el autómata

* Arduino Mega.

* Amperímetro WCS1800 con un amplificador MCP602.

* 4 Voltímetros ZMPT101B con un amplificador LM358.

* Sensores de temperatura DS18B20 1-wire. (Actualmente excluido)

* Módulo SPI to CANbus MCP2515.

* Mosfet IRF520

* Una placa artesanal (en su momento se subirá el esquema).

* Micro botonera de seis botones.

* 12VDC to 9VDC

* 12VDC to 5VDC

* Una recistencia 10K


## Licencia


    Copyright (C) 2019  c2mismo.

    This file is part of 230box.

    230box is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This 230box is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this 230box, see COPYING.  If not, see <https://www.gnu.org/licenses/>.

    You can download a full copy of 230box at <https://github.com/c2mismo>.




  EXCLUDED CODE:
    The code described below and contained in the 230box Source Code,
    is not part of the Program covered by the GPL and is expressly excluded
    from its terms.  You are solely responsible for obtaining from
    the copyright holder a license for such code and complying with
    the applicable license terms.
    THIS SOFTWARE THAT IS DESCRIBED BELOW IS A SOFTWARE CREATED BY
    THIRD PARTIES, THIS EQUIPMENT IS NOT CONTRIBUTED OR MODIFIED
    IN ANY LINE, THEREFORE WE WILL NOT BE LIABLE FOR ANY CLAIM,
    BE FOR DAMAGES OR OTHER LIABILITY.

  DallasTemperature library
---------------------------------------------------------------------------
  lines   files
  885     libraries/DallasTemperature/DallasTemperature.cpp

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.


  OneWire library v2.3
---------------------------------------------------------------------------
  lines   files
  580     libraries/OneWire/OneWire.cpp
  Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
  OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  Except as contained in this notice, the name of Dallas Semiconductor
  shall not be used except as stated in the Dallas Semiconductor
  Branding Policy.


  mcp2515 library
---------------------------------------------------------------------------
  lines   files
  756     lib/mcp2515/mcp2515.cpp
  The MIT License (MIT)

  Copyright (c) 2013 Seeed Technology Inc. Copyright (c) 2016 Dmitry

  Permission is hereby granted, free of charge, to any person obtaining a copy of this
  software and associated documentation files (the "Software"), to deal in the Software 
  ithout restriction, including without limitation the rights to use, copy, modify, merge, 
  ublish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
  to whom the Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or
  substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
  DEALINGS IN THE SOFTWARE.
