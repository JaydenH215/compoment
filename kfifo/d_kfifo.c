#include "d_kfifo.h"



/* size 必须是 2的次幂 */
uint32_t d_kfifo_init(d_kfifo_t d_kfifo, uint8_t *p_buff, uint32_t size) 
{    
    d_kfifo->p_buffer = p_buffer;
    d_kfifo->size     = size;
    d_kfifo->in       = 0;
    d_kfifo->out      = 0;

    return 0;
}

uint32_t __d_kfifo_put(struct d_kfifo_t *d_kfifo, uint8_t *p_buffer, uint32_t len)   
{
    uint32_t L;
    
    //环形缓冲区的剩余容量为fifo->size - fifo->in + fifo->out，让写入的长度取len和剩余容量中较小的，避免写越界；
    len = min(len , d_kfifo->size - d_kfifo->in + d_kfifo->out);

    /* first put the data starting from fifo->in to buffer end */
    /* 首先将数据从fifo.in 所在的位置开始写，写之前，首先要看一下fifo->in到 buffer 末尾的大小 是不是 比 len 大*/

    /*
    * 前面讲到fifo->size已经2的次幂圆整，主要是方便这里计算，提升效率
    * 在对10进行求余的时候，我们发现，余数总是整数中的个位上的数字，而不用管其他位是什么；
    * 所以,kfifo->in % kfifo->size 可以转化为 kfifo->in & (kfifo->size – 1)，效率会提升
    * 所以fifo->size - (fifo->in & (fifo->size - L)) 即位 fifo->in 到 buffer末尾所剩余的长度，
    * L取len和剩余长度的最小值，即为需要拷贝L 字节到fifo->buffer + fifo->in的位置上。
    */ 
    L = min(len, d_kfifo->size - (d_kfifo->in & (d_kfifo->size - 1)));
    
    memcpy(d_kfifo->p_buffer + (d_kfifo->in & (d_kfifo->size - 1)), p_buffer, L);   
  
    /* then put the rest (if any) at the beginning of the buffer */ 

    memcpy(d_kfifo->p_buffer, p_buffer + L, len - L);

    /* 
    * 注意这里 只是用了 fifo->in +=  len而未取模，
    * 这就是kfifo的设计精妙之处，这里用到了unsigned int的溢出性质，
    * 当in 持续增加到溢出时又会被置为0，这样就节省了每次in向前增加都要取模的性能，
    * 锱铢必较，精益求精，让人不得不佩服。
    */
  
    d_kfifo->in += len; 
        
    /*返回值 代表  写入数据的个数 ，这样 就可以根据返回值 判断缓冲区是否写满*/
    return len;   
}  



uint32_t __d_kfifo_get(struct d_kfifo_t *d_kfifo, uint8_t *p_buffer, uint32_t len)   
{
    uint32_t L;   
  
    len = min(len, d_kfifo->in - d_kfifo->out);   

    /* first get the data from fifo->out until the end of the buffer */   
    L = min(len, d_kfifo->size - (d_kfifo->out & (d_kfifo->size - 1)));   
    memcpy(p_buffer, d_kfifo->p_buffer + (d_kfifo->out & (d_kfifo->size - 1)), L);   
  
    /* then get the rest (if any) from the beginning of the buffer */   
    memcpy(p_buffer + L, d_kfifo->p_buffer, len - L);   
  
    /*
    * 注意这里 只是用了 fifo->out +=  len 也未取模运算，
    * 同样unsigned int的溢出性质，当out 持续增加到溢出时又会被置为0，
    * 如果in先溢出，出现 in  < out 的情况，那么 in – out 为负数（又将溢出），
    * in – out 的值还是为buffer中数据的长度。
    */

    d_kfifo->out += len;
  
    return len;  
}


uint32_t __d_kfifo_len(struct d_kfifo_t *d_kfifo)
{
    return d_kfifo->in - d_kfifo->out;
}

uint32_t __d_kfifo_is_full(struct d_kfifo_t *d_kfifo)
{
    return (d_kfifo->size - 1) == (__d_kfifo_len());
}

uint32_t __d_kfifo_is_empty(struct d_kfifo_t *d_kfifo)
{
    return d_kfifo->in == d_kfifo->out;
}