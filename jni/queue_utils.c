//
// Created by xWX371834 on 2017/8/24.
//

#include "queue_utils.h"

void frame_queue_malloc(CustomFrameQueue **queue){
    CustomFrameQueue *_queue = malloc(sizeof(CustomFrameQueue));
    _queue->firstFrame = _queue->lastFrame = NULL;
    _queue->nb_frame = 0;
    _queue->mutex = SDL_CreateMutex();
    _queue->cond = SDL_CreateCond();
    *queue = _queue;
}

void frame_malloc(CustomFrame **frame){
    CustomFrame *_frame = malloc(sizeof(CustomFrame));
    _frame->data = NULL;
    _frame->packet = NULL;
    _frame->width = _frame->height = _frame->index = 0;
    _frame->length = 0;
    _frame->next = NULL;
    *frame = _frame;
}

void frame_queue_put(CustomFrameQueue *queue, CustomFrame *frame, int cache_frame_nb){
    if( queue == NULL ){
        return;
    }
    SDL_LockMutex(queue->mutex);

    if (!queue->lastFrame) {
        queue->firstFrame = frame;
    } else {
        queue->lastFrame->next = frame;
    }
    queue->lastFrame = frame;
    queue->nb_frame++;

    if( queue->nb_frame > cache_frame_nb ){
        SDL_CondSignal(queue->cond);
    }

    SDL_UnlockMutex(queue->mutex);
}

int frame_queue_get(CustomFrameQueue *queue, CustomFrame **frame, int isblock){
    CustomFrame *firstFrame;
    int ret;
    if( queue == NULL ){
        return -3;
    }

    SDL_LockMutex(queue->mutex);
    for (;;) {
        firstFrame = queue->firstFrame;
        if (firstFrame) {
            queue->firstFrame = firstFrame->next;
            if (!queue->firstFrame) {
                queue->lastFrame = NULL;
            }
            firstFrame->next = NULL;
            *frame = firstFrame;
            queue->nb_frame--;
            ret = 1;
            break;
        } else if (!isblock) {
            ret = -1;
            break;
        } else {
            SDL_CondWait(queue->cond, queue->mutex);
        }
    }

    SDL_UnlockMutex(queue->mutex);
    return ret;
}