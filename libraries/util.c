/*All rights reserved.
 *
 * Eigen Technologies Pvt. Ltd.
 * New Delhi
 * www.eigen.in
 *
 * This source code is provided to SensenutsTM  users for
 * research purpose only. No portion of this source code
 * may be used as the basis of development of any similar
 * kind of product.
 *
 */


#include <AppHardwareApi.h>
#include <string.h>
#include <stdbool.h>
#include "node.h"
#include "task.h"
#include "AppQueueApi.h"
#include "config.h"
#include "util.h"



/*
 * Function used send calculate power of the no
 * used to avoid inclusion of math.h to reduce size program
 * return result
 */
 
uint64 power(int x, int y)
{
	uint64 temp=1;
	while (y != 0)
	{
		temp=temp*x;
		y--;
	}
	return temp;
}


/*
 * Function to convert hex string into HEX
 * input is hex string
 * Return Nothing
 */
 
uint8 hexStrToHexVal (uint8 *arr)
{
	uint8 i=0;
	uint8 k=0;
	uint8 app[2];
	for (k=0;k<2;k++)
	{
		switch (arr[k])
		{
			case 'A':
			case 'a':
			{
				app[k] = 0x0a;
				break;
			}
			case 'B':
			case 'b':
			{
				app[k] = 0x0b;
				break;
			}
			case 'C':
			case 'c':
			{
				app[k] = 0x0c;
				break;
			}
			case 'D':
			case 'd':
			{
				app[k] = 0x0d;
				break;
			}
			case 'E':
			case 'e':
			{
				app[k] = 0x0e;
				break;
			}
			case 'F':
			case 'f':
			{
				app[k] = 0x0f;
				break;
			}
			default:
			{
				if (arr[k] >= 0x30 && arr[k] <= 0x39)
				{
					app[k] = arr[k]-48;
					if (arr[k] > 9)
						arr[k]=9;
				}
				break;
			}
		}
	}
	i = app[0];
	i = i<<4;
	uint8 result = i|app[1];
	return result;
}
 


/*
 * Function to calculate and convert interger into string
 * it modifies the main string
 * input is the interger value
 */

 
 
uint8 intToStr (int numb, uint8 *str)
{
	uint32 num=numb;
	int32 i=0;
	int32 j=0;
	int32 v=0;
	if (num == 0x00)
	{
		str[0] = '0';
		return 1;
	}
	
	if (num<0)
	{
	    str[v]='-';
	    v++;
	}    
	
	//calculate no of array digits we needs
	while (num/power(10,i) != 0)
		i++;
	//Put all digit one by one to array
	for (j=0;j<i;j++)
	{
		str [v+i-j-1] = num%10+48;
		num = num/10;
	}
	return i+v;
}


/*
 * Function to convert float data into string
 * input is float value and base address of ASCII string
 * Return -- String Size
 */
 
uint8 floatToStrConverter (float num, uint8 *str)
{
	uint32 i=0;
	uint32 j=0;
	uint32 v=0;
	if (num<0)
	{
	    num=num*(-1);
	    str[v]='-';
	    v++;
	} 
	//extract integer part from num
	int intData = num;
	//extract float part from num
	float floatData = num-intData;
	//convert float data into int 
	floatData = floatData*100.1;
	//convert it into int 
	int intData2 = floatData;
	//calculate int part length in ASCII
	while (intData/power(10,i) != 0)
		i++;
	if (intData == 0)
	{
		str [v] = intData%10+48;
		i=1;
	}
	else{
		//copy complete int data into string
		for (j=0; j<i; j++)
		{
			str [v+i-j-1] = intData%10+48;
			intData = intData/10;
		}
	}
	str [v+i] = '.';
	//copy two decimal palce float data into string
	for (j=i+2;j>i;j--)
	{
		str [v+j] = intData2%10+48;
		intData2 = intData2/10;
	}
	return (v+i+3);
}



/** 
 **
 Function to convert hexadecimal value into string
 Input is pointer to the based address of data buffer
 and pointer to save data into Buffer
 and length of data 
 Auther -- Tech Team Eigen Lighting
 **
 **/
 

void hexToStrConverter (uint8 *hexData, uint8 *strData, int len)
{
	int i=0;
	uint8 upperNib;
	uint8 lowerNib;
	for (i=0; i<len; i++)
	{
		lowerNib = (hexData[i] & 0x0F);
		if (lowerNib < 0x0a)
			lowerNib+=0x30;
		else
			lowerNib+=55;
		upperNib = hexData[i] & 0xF0;
		upperNib = (upperNib >>4);
		if (upperNib < 0x0a)
			upperNib +=0x30;
		else
			upperNib +=55;
		strData [2*i]= upperNib;
		strData [2*i+1]= lowerNib;
		
	}
	
}

/** 
 ** 
 Function to convert string into corresponding in value
 Return -- double value from string
 Author -- Tech Team Eigen Lighting
 **
 **/
 
uint64 stringToIntConverter (uint8 *ptr, uint8 size)
{
	uint64 finalValue =0;
	while (size != 0x00)
	{
		finalValue += power(10,(size-1))*(*ptr-0x30);
		ptr++;
		size --;
	}
	return finalValue;
}
	
	
//Function to convert a hex string into equivalent hex num
uint8 hexStringToHexNumConverter (uint8 *hexData, uint8 *hexString, uint8 hexStringLen)
{
    if (((hexStringLen%2) != 0) || (hexStringLen>254) || ((hexStringLen == 1)))
        return 0;
    int i = 0;
    int j = 0;
    for (i=0; i<hexStringLen;)
    {
        hexData [j] = hexStrToHexVal (&hexString[i]);
        i +=2;
        j++;
    }
    return j;
}

/*
void printFormattedMsg (const char *insertedString, uint8 *dataString, uint16 dataStringSize)
{
	if (dataStringSize > 254)
	{
		updateNodePktToGateway (dataString, dataStringSize);
		return;
	}
	uint8 packet [MAX_GATEWAY_PKT_SIZE];
	uint16 packetIndex = 0;
	packet[packetIndex] = PRINT_LIKE_MSG_HEADER;
	packetIndex ++;
	uint8 stringSize = strlen (insertedString);
	packet[packetIndex] = stringSize;
	packetIndex ++;
	memcpy (&packet[packetIndex], &insertedString[0], stringSize);
	packetIndex += stringSize;
	
	packet[packetIndex] = dataStringSize;
	packetIndex ++;
	memcpy (&packet[packetIndex], &dataString[0], dataStringSize);
	packetIndex += dataStringSize;
	updateNodePktToGateway (packet, packetIndex);
	
}
	*/
