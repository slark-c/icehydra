# The Examples to Use Icehydra

## 

## Background

First,you need to start a background process for the broker.

- icehydra  -f  your.json -D

Icehydra use json file to config Publisher and  Subscriber.

## JSON File

The json file tells how icehydra works , like:

```
{
    "ih-name":"test2-Publisher",
    "ih-pid"  : 0,
    "ih-shm":[
        {
         "shm_name":"name1",
         "shm_size":128
        },
        {
         "shm_name":"testname2",
         "shm_size":256
        },
        {
         "shm_name":"testname3",
         "shm_size":512
        },
        {
         "shm_name":"testname4",
         "shm_size":1024
        }
    ],
    "ih-cpu_bind":0
}
{
    "ih-name":"test2-Subscriber-1",
    "ih-pid"  : 100
}
{
    "ih-name":"test2-Subscriber-2",
    "ih-pid"  : 101
}
```

The first json node confiures the name and bound CPU ID of the Publisher, and initializes four shared memory bloks.Note that the Publisher's "ih-pid" must be 0.

The remaining processes connecting to the Publisher only need to configure the "ih-pid" and name.

**NOTE: ih-pid ranges from 0-255. Indeed, 0 must be reserver for the  Publisher**.

Subscriber uses 1-255.

## System

After completing the previous step,when you input '*ipcs*' , you should see the following information.

![The ipcs img][ipcs-img]

[ipcs-img]:ipcs-info.JPG

## Subscriber

The subscriber process is easy to connect to the broker .

```
//create and connect to the broker,use the name configured in json file
ih_subscriber_create_connect("test2-Subscriber-1","test2-Publisher");

//wait other subscriber process
//this is a blocking function
ih_wait_other_process();
```

Then you can send messages  through send-apis

```
//uint8_t ids_array = {101} (it's the ih-pid defined in json you want to send-to)
//ids_num = sizeof(ids_array)
IH_BROADCAST_CMD_T cmdtest = {
        .br_ids = ids_array,
        .br_num = ids_num,
        .data = str,
        .datalen = strlen};

ret = ih_send_broadcast_data(&cmdtest);    
```

The another subscriber process will recv the str by  recv-api

```
while(1){
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(!ih_select_recv_ready(&timeout))
        continue;

    ret = ih_recv_data(recvbuf,&recvlen);
    if(ret < 0){
        exit(-1);
    }
}
```

## Share Memory

But how we use the share memory we defined ?

1. First,we send a get-shm-cmd to the Publisher to get all share memory id.

2. Then use recv-api check is get-shm-cmd-reply.

3. Broker return the IDs of shared memory in the order defined by the json file.

4. Finally,use API to attch to share memory

```
//want to use share memory,send cmd to broker
ih_send_getShmIDs_cmd();

while(1){
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    if(!ih_select_recv_ready(&timeout))
        continue;

    ret = ih_recv_data(recvbuf,&recvlen);
    if(ret < 0){
        exit(-1);
    }

    //check recv type is share-memory-ids
    if(ih_recvIsShmIDs(ret)){
        //get share memory ptr by shm_name in JSON
        ih_get_shm_by_name(recvbuf,"testname1",&shm_ptr);
    }
}
```