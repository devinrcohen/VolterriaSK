//
//  ContentView.swift
//  VolterriaSK
//
//  Created by Devin R Cohen on 12/7/25.
//

import SwiftUI
import SpriteKit

struct ContentView: View {
    @State private var scene: GameScene?
    
    var body: some View {
        GeometryReader { proxy in
            let size = proxy.size
            
            ZStack {
                if let scene {
                    SpriteView(scene: scene)
                        //.ignoresSafeArea()
                } else {
                    Text("Loading Volterria...")
                        .frame(maxWidth: .infinity, maxHeight: .infinity)
                }
            }
            .onAppear {
                if scene == nil {
                    createScene(for: size)
                }
            }
        }
    }
    
    private func createScene(for size: CGSize) {
        // Construct engine with defaults
        var engine = VolterriaEngine()
        
        // If C++ side has reset/seed method, call it here:
        engine.ResetSimulation()
        
        // Create SpriteKit Scene
        let gameScene = GameScene(size: size, engine: engine)
        //gameScene.anchorPoint = CGPoint(x: 0.5, y: 0.5)
        // gameScene.anchorPoint = CGPoint(x: 0.0, y: 0.0)
        // Store it so SwiftUI doesn't recreate it every body() call
        self.scene = gameScene
    }
}

#Preview {
    ContentView()
}
