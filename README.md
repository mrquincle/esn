<!-- Uses markdown syntax for neat display at github -->

# ESN
ESN stands for Echo State Network.

## What does it do?
Echo state networks can be used for prediction (but not only for that). It belongs to the class of so-called reservoir computing methods. 

## Is it good?
Echo state machines (as other reservoir computing methods) uses a network of recurrently connected neurons with fixed (or quenched) weights. The intrinsic dynamics of the network reflects the input. Stability is guaranteed by (down)scaling all the weights by the same factor (using the spectral radius of the connectivity matrix). This leads to fading activity in all cases: whatever input pattern is offered to the network, its activity will never increase in such way that it becomes unstable. The performance of the network depends on the number of neurons (but does not quantitatively scale with it). The learning part resides in a straightforward readout layer that maps all the states of the neurons in the reservoir to the outputs. In the case of prediction this might be one output neuron. In the case of classification it will be multiple neurons, for example one per class. The readout layer is trained by linear regression (implemented using two very small third-party libraries: "eigenvalues" and "inv").

## What are the alternatives?
Almende and DoBots have been using ESNs for audio processing in the [Robot3D](https://launchpad.net/robot3d) simulator (an open-source multi-robot simulator using in the European research project Replicator). ESNs performs quite okay on simple tasks like vowel identification and recognition of simple audio signatures. Do not expect very well performance on temporal data that is a.) context-sensitive, b.) hierarchically structured, or c.) spanning multiple time-scales. Alternatives to echo state networks are temporal recurrent neural networks, liquid state machines, and decorrelation-backpropagation learning.

## An example
An often used example to showcase the prediction properties of a method is the chaotic time-series corresponding to the time delay differential Mackey-Glass equation. Depending on the parameters the exhibited behaviour ranges from periodic to chaotic, hence it can function to demonstrate the strengths of a predictor. Below you can see the time series prediction over a short and longer time span. The last picture shows the state of the echo state network.

![Mackey-Glass prediction](https://github.com/mrquincle/esn/raw/master/doc/mackey_glass.png "Mackey-Glass time series prediction")

![Long Mackey-Glass prediction](https://github.com/mrquincle/esn/raw/master/doc/mackey_glass_long.png "Mackey-Glass time series prediction over longer time span")

![ESN state](https://github.com/mrquincle/esn/raw/master/doc/reservoir_state.jpg "Echo state network state")

## What does this use?

There are no dependencies except for the normal ones available to each Linux system. The directory `thd` contains the code for calculating eigenvectors required for the pseudo-inverse. That code comes from the [AP Library adapted for C++](http://stuff.mit.edu/afs/athena/course/16/16.225/dv/VTK/Utilities/vtkalglib/ap.english.html) at MIT which is provided through [alglib](http://www.alglib.net/). If you like feel free to replace it with Eigen. Only its vector and matrix representations are used, with perhaps a determinant so here and there besides the eigenvectors (I forgot).

## Where can I read more?
* [Scholarpedia (on ESNs)](http://www.scholarpedia.org/article/Echo_state_network)
* [Scholarpedia (on Mackey-Glass)](http://www.scholarpedia.org/article/Mackey-Glass_equation)

## Copyrights
The copyrights (2014) belong to:

- Author: Ted Schmidt
- Author: Anne van Rossum
- Almende B.V., http://www.almende.com and DoBots, http://www.dobots.nl
- Rotterdam, The Netherlands
