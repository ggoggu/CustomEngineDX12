# DirectX 12 Core Safety & Resource Management Rules

DirectX 12 is an explicit API where the developer is responsible for synchronization, memory residency, and state transitions. Violating these rules results in GPU crashes (TDR), screen tearing, flickering, or silent corruption.

---

## 1. Resource State Transitions & Barriers

1. **Explicit Transitions**:
   - Every resource must be transitioned to its required state before being accessed by the GPU.
   - Example backbuffer lifecycle:
     ```cpp
     // Present -> Render Target
     D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
         m_renderTargets[m_frameIndex].Get(),
         D3D12_RESOURCE_STATE_PRESENT,
         D3D12_RESOURCE_STATE_RENDER_TARGET
     );
     m_commandList->ResourceBarrier(1, &barrier);
     
     // ... Draw calls ...
     
     // Render Target -> Present
     barrier = CD3DX12_RESOURCE_BARRIER::Transition(
         m_renderTargets[m_frameIndex].Get(),
         D3D12_RESOURCE_STATE_RENDER_TARGET,
         D3D12_RESOURCE_STATE_PRESENT
     );
     m_commandList->ResourceBarrier(1, &barrier);
     ```

2. **UAV Barriers (Read-After-Write / Write-After-Write)**:
   - When a resource is written via an Unordered Access View (UAV) and then read or written again in a subsequent dispatch (e.g. accumulation buffers, raytracing output), an explicit UAV barrier is mandatory:
     ```cpp
     D3D12_RESOURCE_BARRIER uavBarrier = {};
     uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
     uavBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
     uavBarrier.UAV.pResource = m_accumulationBuffer.Get();
     m_commandList->ResourceBarrier(1, &uavBarrier);
     ```

3. **Subresource Indexing**:
   - Default to `D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES` unless transitioning a specific mip slice or array slice.

---

## 2. Buffer Lifetimes & Upload Heap Invariants

1. **Upload Buffer Lifetime**:
   - Resources created on `D3D12_HEAP_TYPE_UPLOAD` (used to stage vertex, index, or texture data into `D3D12_HEAP_TYPE_DEFAULT`) **must not be released** until the command list that reads them has finished execution on the GPU.
   - Releasing an upload buffer while the command list is queued or executing produces an immediate GPU page fault or corrupted mesh.
   - Retain `vertexBufferUpload` and `indexBufferUpload` as member variables or in a pending-release queue until `WaitForGpu()` or `FlushCommandQueue()` completes.

2. **Constant Buffer Mapping**:
   - Constant buffers in upload heaps can remain persistently mapped via `Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin))` until engine shutdown.
   - Write memory with `memcpy` from CPU and make sure the struct size is aligned to 256 bytes (`(sizeof(CB) + 255) & ~255`).

---

## 3. CPU-GPU Synchronization & Fences

1. **Command Allocator Safety**:
   - **Never** call `m_commandAllocator->Reset()` while the GPU is executing commands recorded from that allocator.
   - Command allocators can only be reset *after* the fence value associated with their execution has been signaled and waited on.

2. **Command List Reset**:
   - `m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get())` can be called as soon as the command list is closed and submitted with `ExecuteCommandLists`.

3. **Flush & Wait Routine**:
   ```cpp
   void DX12Renderer::FlushCommandQueue()
   {
       m_fenceValue++;
       ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), m_fenceValue));
       if (m_fence->GetCompletedValue() < m_fenceValue)
       {
           ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent));
           WaitForSingleObject(m_fenceEvent, INFINITE);
       }
   }
   ```

---

## 4. Descriptor Heap Management

1. **Heap Types & Separation**:
   - `D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV`: Only bindable to graphics/compute pipelines when flagged with `D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE`.
   - `D3D12_DESCRIPTOR_HEAP_TYPE_RTV` and `DSV`: Must **never** be shader visible.
   - At most one CBV_SRV_UAV heap and one Sampler heap can be bound simultaneously with `SetDescriptorHeaps()`.

2. **Handle Calculation**:
   - Always query descriptor increment sizes dynamically using:
     `m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_...)`
   - Never hardcode descriptor handle byte strides.
