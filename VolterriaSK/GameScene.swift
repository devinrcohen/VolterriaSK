//
//  GameScene.swift
//  VolterriaSK
//
//  Created by Devin R Cohen on 12/7/25.
//

import SpriteKit

final class GameScene: SKScene {
    // Engine & Timing
    //private let engine: VolterriaEngine
    private var engine: VolterriaEngine
    private var lastUpdateTime: TimeInterval = 0.0
    
    // Node pools
    private var preyNodes: [Int32: SKShapeNode] = [:]
    private var predatorNodes: [Int32: SKShapeNode] = [:]
    private var grassNodes: [Int32: SKShapeNode] = [:]
    private var grassLabelNodes: [Int32: SKLabelNode] = [:]
    
    private let populationLabel = SKLabelNode() // nil for system font
    //private var grassLabels: [Int32: SKLabelNode] = [:]
    // Init
    init(size: CGSize, engine: VolterriaEngine) {
        self.engine = engine
        super.init(size: size)
        self.scaleMode = .resizeFill
        self.anchorPoint = CGPoint(x: 0.0, y: 0.0)
        backgroundColor = .black
    }
    
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }
    
    // Scene lifecycle
    override func didMove(to view: SKView) {
        self.backgroundColor = .black
        
        populationLabel.fontSize = 24.0
        populationLabel.fontColor = .white
        populationLabel.horizontalAlignmentMode = .left
        populationLabel.verticalAlignmentMode = .top
        populationLabel.position = CGPoint(x: 8, y: size.height - 8)
        addChild(populationLabel)
    }
    
    override func update(_ currentTime: TimeInterval) {
        // compute dt
        if lastUpdateTime == 0 {
            lastUpdateTime = currentTime
            return // this is new as of grid implementation
        }
        let dt_raw = currentTime - lastUpdateTime
        let dt = min(dt_raw, 1.0/60.0) // clamps spikes
        lastUpdateTime = currentTime
        
        // 1.  Step C++ Engine
        engine.step(dt)
        
        // 2.  Pull snapshots
        let creaturesVec = engine.creatureSnapshot()
        let creatures = Array(creaturesVec)
        let grassPatchesVec = engine.grassSnapshot()
        let grassPatches = Array(grassPatchesVec)
        
        // 3.  Sync SpriteKit nodes to snapshot
        syncGrassNodes(to: grassPatches)
        syncCreatureNodes(to: creatures)
        
        // Update label
        let preyCount = creatures.filter{ $0.role == .Prey }.count
        let predatorCount = creatures.filter{ $0.role == .Predator }.count
        populationLabel.text = "Prey: \(preyCount)\n Predators: \(predatorCount)"
    }
    
    private var worldXMin: CGFloat { CGFloat(engine.worldXMin()) }
    private var worldXMax: CGFloat { CGFloat(engine.worldXMax()) }
    private var worldYMin: CGFloat { CGFloat(engine.worldYMin()) }
    private var worldYMax: CGFloat { CGFloat(engine.worldYMax()) }
    private var worldWidth: CGFloat { CGFloat(engine.worldWidth())}
    private var worldHeight: CGFloat { CGFloat(engine.worldHeight())}
    
    private func mapToSceneX(_ x: Float) -> CGFloat {
        guard worldWidth > 0 else { return size.width / 2 }
        
        let t = (CGFloat(x) - worldXMin) / worldWidth
        return t * size.width
    }
    private func mapToSceneY(_ y: Float) -> CGFloat {
        guard worldHeight > 0 else { return size.height / 2 }
        
        let t = (CGFloat(y) - worldYMin) / worldHeight
        return (1.0 - t) * size.height // flip vertically
    }
    
    private var patchScale: CGFloat {
        // keep aspect ratio, scale radius
        guard worldWidth > 0, worldHeight > 0 else { return 1.0 }
        let sx = size.width / worldWidth
        let sy = size.height / worldHeight
        return min(sx, sy)
    }
    
    private func syncCreatureNodes(to creatures: [VCreatureSnapshot]) {
        var seenPrey = Set<Int32>()
        var seenPredators = Set<Int32>()
        
        for c in creatures {
            let pos = CGPoint(
                x: mapToSceneX(c.x),
                y: mapToSceneY(c.y)
            )
            switch c.role {
            case .Prey:
                seenPrey.insert(c.id)
                let node = preyNodes[c.id] ?? makePreyNode(id: c.id, sex: c.sex)
                node.position = pos
            case .Predator:
                seenPredators.insert(c.id)
                let node = predatorNodes[c.id] ?? makePredatorNode(id: c.id, sex: c.sex)
                node.position = pos
            @unknown default:
                break
            }
        }
        
        // Despawn nodes that disappeared (aka have no ID)
        for (id, node) in preyNodes where !seenPrey.contains(id) {
            node.removeFromParent()
            preyNodes.removeValue(forKey: id)
        }
        for (id, node) in predatorNodes where !seenPredators.contains(id) {
            node.removeFromParent()
            predatorNodes.removeValue(forKey: id)
        }
    }
    
    private func makePreyNode(id: Int32, sex: VSex) -> SKShapeNode {
        let radius: CGFloat = 4
        let node = SKShapeNode(circleOfRadius: radius)
        node.fillColor = .green
        switch sex {
        case .Male:
            node.strokeColor = .blue
        case .Female:
            node.strokeColor = .magenta
        @unknown default:
            node.strokeColor = .clear
        }
        addChild(node)
        preyNodes[id] = node
        return node
    }
    
    private func makePredatorNode(id: Int32, sex: VSex) -> SKShapeNode {
        let radius: CGFloat = 10
        let node = SKShapeNode(circleOfRadius: radius)
        node.fillColor = .red
        switch sex {
        case .Male:
            node.strokeColor = .blue
        case .Female:
            node.strokeColor = .magenta
        @unknown default:
            node.strokeColor = .white
        }
        addChild(node)
        predatorNodes[id] = node
        return node
    }
    
    private func syncGrassNodes(to patches: [VGrassPatchSnapshot]) {
        var seen = Set<Int32>()
        
        for g in patches {
            seen.insert(g.id)
            
            let grassPos = CGPoint(
                x: mapToSceneX(g.x),
                y: mapToSceneY(g.y)
            )
            
            let grassLabelPos = CGPoint(
                x: grassPos.x,
                y: grassPos.y
            )
            
            let grassNode = grassNodes[g.id] ?? makeGrassNode(id: g.id)
            let grassLabelNode = grassLabelNodes[g.id] ?? makeGrassLabelNode(id: g.id)
            grassNode.position = grassPos
            
            let formattedHealth = String(format: "%.2f", g.health)
            grassLabelNode.text = "\(formattedHealth)"
            grassLabelNode.position = grassLabelPos
            
            let screenRadius = CGFloat(g.radius) * patchScale
            grassNode.path = CGPath(
                ellipseIn: CGRect(
                    x: -screenRadius,
                    y: -screenRadius,
                    width: screenRadius * 2.0,
                    height: screenRadius * 2.0
                ),
                transform: nil
            )
            
            grassNode.fillColor = grassColor(for: g.normalizedHealth)
        }
        
        // Remove dead patches
        for(id, grassNode) in grassNodes where !seen.contains(id) {
            grassNode.removeFromParent()
            grassNodes.removeValue(forKey: id)
        }
    }
    
    private func makeGrassNode(id: Int32) -> SKShapeNode {
        let node = SKShapeNode(circleOfRadius: 1.0)
        node.strokeColor = .clear
        addChild(node)
        grassNodes[id] = node
        return node
    }

    private func makeGrassLabelNode(id: Int32) -> SKLabelNode {
        let node = SKLabelNode(text: "\(id)")
        addChild(node)
        grassLabelNodes[id] = node
        return node
    }
    
    private func grassColor(for normalizedHealth: Float) -> SKColor {
        // teal fade -- clamp health first
        let h = max(0.0, min(1.0, CGFloat(normalizedHealth)))
        return SKColor(
            red: 0.0,
            green: 0.7 * h,
            blue: 0.7,
            alpha: 0.2 + 0.6 * h
        )
    }
}


